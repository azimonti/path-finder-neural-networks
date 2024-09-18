/************************/
/*  cs_scenario.cpp     */
/*    Version 1.0       */
/*     2022/06/24       */
/************************/

#include <algorithm>
#include <filesystem>
#include <functional>
#include <future>
#include <numeric>
#include <random>
#include "hdf5/hdf5_ext.h"
#include "log/log.h"
#include "cs_math.h"
#include "cs_modelfactory.h"
#include "cs_player.h"
#include "cs_scenario.h"
#include "cs_serialize.h"
#include "cs_terrain.h"
#include "cs_trainer.h"
#include "cs_unit.h"
#include "cs_utils.h"
#include "ge_mesh2.h"
#include "mesh.h"
#include "plasma2.h"
#include "scene.h"
#include "uibase.h"
#include "utils.h"

#define TRAIN_SINGLE_TERRAIN

static const std::string CS_CONFIG_FNAME = ".cs_config.json";

static auto localLog                     = [](const char* ftm, ...) {
    char buffer[2048]{};
    va_list args;
    va_start(args, ftm);
    vsnprintf(buffer, sizeof(buffer), ftm, args);
    va_end(args);
    LOGGER(logging::INFO) << buffer;
};

static CS_Sim::Params makeDefaultSimParams(const CS_Terrain& terr)
{
    const auto fieldSize = terr.GetFieldSize();
    CS_Sim::Params par;
    par.mInitUnitsN  = 1;
    par.mMaxTimeS    = 60 * 15;

    // check with some
    auto isGoodPoint = [&](const glm::vec3& pos) {
        for (int ix = 0; ix <= 30; ix += 3)
            for (int iz = 0; iz <= 30; iz += 3)
            {
                const auto sx     = ((ix & 1) * 2 - 1) * (ix / 2);
                const auto sz     = ((iz & 1) * 2 - 1) * (iz / 2);
                const auto fx     = (float)sx * terr.GetCellSize();
                const auto fz     = (float)sz * terr.GetCellSize();
                const auto posOff = pos + glm::vec3(fx, 0.f, fz);

                if (!terr.IsPosInside(posOff)) return false;

                if (terr.GetHeightFromPos(posOff) >= CS_Sim::GetWallHeight_s()) return false;
            }
        return true;
    };

    auto pickValidPos = [&](const glm::vec3& center) {
        if (isGoodPoint(center)) return center;

        for (int d = 1; d < 20; ++d)
        {
            const auto df = terr.GetFieldSize() * 0.5f * ((float)d / 20.f);
            for (int a = 0; a < 200; ++a)
            {
                const auto af  = (float)(2 * glm::pi<float>() * (float)a) / 200.f;
                const auto pos = center + glm::vec3(cosf(af), 0, sinf(af)) * df;
                if (isGoodPoint(pos)) return pos;
            }
        }
        assert(0);
        return center;
    };

    const auto border = 0.05f;
    par.mStartPos     = pickValidPos(glm::vec3((0.5f - border), 0, (0.5f - border)) * fieldSize);
    par.mTargetPos    = pickValidPos(glm::vec3(-(0.5f - border), 0, -(0.5f - border)) * fieldSize);

    return par;
}

static void to_json(nlohmann::json& j, const CS_Scenario& v)
{
    j = nlohmann::json{
        CS_SERIALIZE_VAL(mInitUnitsN),
        CS_SERIALIZE_VAL(mCurModelIdx),
        {"mTrain::Setup", v.msTrain->MakeSetup()},
    };
}

static void from_json(const nlohmann::json& j, CS_Scenario& v)
{
    CS_DESERIALIZE_VAL(mInitUnitsN);
    CS_DESERIALIZE_VAL(mCurModelIdx);
    if (auto it = j.find("mTrain::Setup"); it != j.end())
        v.msTrain = std::make_unique<CS_ScenarioTrain>(it->get<CS_ScenarioTrain::Setup>());
}

CS_Scenario::CS_Scenario()
{
    // common initial terrain setup
    CS_Terrain::Params tpar;
    tpar.tp_fieldSize        = 100.f;
    tpar.tp_useImage         = false;
    tpar.tp_noise_barrierLev = 0.6f;
    tpar.tp_noise_seed       = 102;
    tpar.tp_noise_roughness  = 0.50f;

    // set for testing
    mTest.mTerrSetup         = {tpar, {102}};

    // set for training
    msTrain                  = std::make_shared<CS_ScenarioTrain>();
#ifdef TRAIN_SINGLE_TERRAIN
    msTrain->mTerrSetup = {tpar, {102}};
#else
    msTrain->mTerrSetup = {tpar, {102, 104, 106, 108, 110});
#endif

    readConfig();
}

CS_Scenario::~CS_Scenario()
{
    writeConfig();
}

void CS_Scenario::stopTesting()
{
    moCurBrain.reset();
    moCurSim.reset();
}

void CS_Scenario::readConfig()
{
    // read the json config file and deserialize it
    if (std::filesystem::exists(CS_CONFIG_FNAME))
    {
        try
        {
            // read file to std::string, deserialize std::string
            std::ifstream ifs(CS_CONFIG_FNAME);
            std::string str((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            nlohmann::json j = nlohmann::json::parse(str);
            from_json(j, *this);
        } catch (const std::exception& e)
        {
            localLog("Failed to read config file: %s", e.what());
        }
    }
}

void CS_Scenario::writeConfig() const
{
    try
    {
        nlohmann::json j;
        to_json(j, *this);
        // serialize to std::string, write to file
        std::ofstream ofs(CS_CONFIG_FNAME);
        ofs << j.dump(2);
    } catch (const std::exception& e)
    {
        localLog("Failed to write config file: %s", e.what());
    }
}

void CS_Scenario::reqWriteConfig()
{
    // request to write the config in 1 second
    mWriteConfigTimeS = ut::GetSteadyTimeS() + 1;
}

void CS_ScenarioTest::AnimateSceTest(bool doDraw)
{
    // always have a terrain up and showing
    if (!moTerr) moTerr = std::make_unique<CS_Terrain>(mTerrSetup.MakeVariants().front());

    if (moPlayer) moPlayer->AnimPlayer(mSpeedFactor, doDraw);
}

void CS_Scenario::AnimScenario(bool doDraw)
{
    if (mWriteConfigTimeS > 0 && ut::GetSteadyTimeS() > mWriteConfigTimeS)
    {
        mWriteConfigTimeS = 0;
        writeConfig();
    }

    mTest.AnimateSceTest(doDraw);

    msTrain->AnimateSceTrain();
}

void CS_Scenario::AddMeshesToScene(ge::Scene& scene)
{
    mTest.moTerr->AddMeshesToSceneTerr(scene);

    if (mTest.moPlayer)
    {
        auto& sim = mTest.moPlayer->GetPlayerSim();
        sim.AddMeshesToSceneSim(scene);
        const auto staCol = glm::vec4(0.0f, 0.8f, 0.0f, 1);
        const auto tarCol = glm::vec4(0.8f, 0.0f, 0.0f, 1);
        mTest.moTerr->DI_DrawCellAtPos(sim.mPars.mStartPos, staCol, staCol);
        mTest.moTerr->DI_DrawCellAtPos(sim.mPars.mTargetPos, tarCol, tarCol);
    }

    mTest.moTerr->DI_Flush();
}

void CS_Scenario::OnPickedMeshSce(const ge::Mesh* pMesh)
{
    if (mTest.moPlayer) mTest.moPlayer->GetPlayerSim().OnPickedMeshSim(pMesh);
}

void CS_Scenario::draw_TrainUI()
{
    // draw terrain setup and see if we need to change it
    if (msTrain->mTerrSetup.DrawTerrSetupUI(true))
    {
        // do nothing until we start training
    }

    // handle training
    if (msTrain->moTrainer)
    {
        if (ImGui::Button("Stop Training")) { msTrain->moTrainer->ReqShutdown(); }
        ImGui::SameLine();
        ImGui::Text("Training epoch:%zu...", msTrain->moTrainer->GetCurEpochN());
    }
    else
    {
        if (ImGui::Button("Start Training"))
        {
            const auto variants = msTrain->mTerrSetup.MakeVariants();
            // create one terrain for each simulation scenario
            for (size_t i = 0; i < variants.size(); ++i)
            {
                const auto& terrPar = variants[i];
                msTrain->moTerrs.push_back(std::make_unique<CS_Terrain>(terrPar));

                auto simPar        = makeDefaultSimParams(*msTrain->moTerrs.back());
                simPar.mInitUnitsN = 1;
                msTrain->mSimPars.push_back(simPar);
            }

            CS_Trainer::Params par;
            par.maxEpochsN  = 5000;

            par.evalBrainFn = [&simPars = msTrain->mSimPars,
                               &terrs   = msTrain->moTerrs](const CS_BrainBase& brain, std::atomic<bool>& reqShutdown) {
                double totCost = 0;
                for (size_t sidx = 0; sidx < simPars.size(); ++sidx)
                {
                    // create a simulation for the given scenario and brain
                    auto oSim = std::make_unique<CS_Sim>(simPars[sidx], *terrs[sidx], brain, false);

                    // run to completion (includes timeout)
                    while (!oSim->IsSimComplete() && !reqShutdown) oSim->AnimSim(1.0 / 60.0, false);

                    totCost += oSim->GetAvgTotalCost();
                }

                return totCost / static_cast<double>(simPars.size());
            };

            // create the trainer
            msTrain->moTrainer = std::make_unique<CS_Trainer>(
                par, CS_ModelFactory::CreateTrain(mCurModelIdx, (size_t)CS_SENS_N, (size_t)CS_CTRL_N));

            msTrain->mLastEpoch      = 0;
            msTrain->mLastEpochTimeS = ut::GetSteadyTimeS();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(UIB_ContentSca * 150);
        if (ImGui::BeginCombo("##Model", CS_ModelFactory::GetModelName(mCurModelIdx).c_str()))
        {
            const auto n = CS_ModelFactory::GetModelsN();
            for (size_t i = 0; i < n; ++i)
            {
                const bool isSelected = (mCurModelIdx == i);
                if (ImGui::Selectable(CS_ModelFactory::GetModelName(i).c_str(), isSelected))
                {
                    mCurModelIdx = i;
                    reqWriteConfig();
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }

    if (msTrain->moTrainer)
    {
        if (msTrain->mLastEpochLenTimeS)
        {
            ImGui::Text("Epoch time: %.1fs", msTrain->mLastEpochLenTimeS);
            ImGui::Text("Epochs per hour: %.1f", 60 * 60 / msTrain->mLastEpochLenTimeS);
        }
        else
        {
            ImGui::Text("Epoch time: -");
            ImGui::Text("Epochs per hour: -");
        }
    }

    if (UIB_Header("Brains", true, true))
    {
        static size_t SHOW_TOP_N = 20;
        if (ImGui::BeginTable("TopBrains", 5, ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableHeadersRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", "");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("Epoch");
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("Num");
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("Cost");

            // copy the latest best chromos
            if (msTrain->moTrainer)
                msTrain->moTrainer->LockViewBestChromos([this](const auto& chromos, const auto& infos) {
                    mBestChromos = chromos;
                    mBestCInfos  = infos;
                });

            for (size_t i = 0; i < std::min(SHOW_TOP_N, mBestCInfos.size()); ++i)
            {
                const auto& ci = mBestCInfos[i];

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%zu", i);
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%zu", ci.ci_epochIdx);
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%zu", ci.ci_popIdx);
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%s", CS_MakeCostString(ci.ci_cost).c_str());
                ImGui::TableSetColumnIndex(4);
                if (ImGui::Button(("Test##" + std::to_string(i)).c_str()))
                {
                    CS_Player::Params par;
                    par.chromoCost  = ci.ci_cost;
                    par.chromoHex   = mBestChromos[i].ToHashHex();
                    par.createSimFn = [this](const CS_BrainBase& brain) {
                        auto simPar        = makeDefaultSimParams(*mTest.moTerr);
                        simPar.mInitUnitsN = 1;
                        return std::make_unique<CS_Sim>(simPar, *mTest.moTerr, brain, true);
                    };

                    mTest.moPlayer = std::make_unique<CS_Player>(
                        par, CS_ModelFactory::CreateBrain(mCurModelIdx, mBestChromos[i], CS_SENS_N, CS_CTRL_N));
                }
            }
            ImGui::EndTable();
        }
    }
}

void CS_ScenarioTest::DrawTestUI()
{
    // draw terrain setup and see if we need to change it
    if (mTerrSetup.DrawTerrSetupUI(false))
    {
        // make sure to reset the player first, since it relies on the terrain
        moPlayer.reset();
        moTerr = std::make_unique<CS_Terrain>(mTerrSetup.MakeVariants().front());
    }

    ImGui::SetNextItemWidth(ImGui::GetWindowWidth() * 0.5f);
    UIB_SlideDouble("Speed", &mSpeedFactor, 1.0, 20.0, "%.1f");

    if (moPlayer)
    {
        ImGui::Text("Testing...");
        ImGui::Text("Chromo: %s", moPlayer->GetChromoHex().c_str());
        ImGui::Text("Cost: %s", CS_MakeCostString(moPlayer->GetChromoCost()).c_str());

        moPlayer->GetPlayerSim().DrawSimUI();
    }
    else
    {
        ImGui::Text(" ");
        ImGui::Text(" ");
        ImGui::Text(" ");
    }
}

void CS_Scenario::DrawScenarioUI(ge::Camera& cam, CH::CamHandMouse& camHand)
{
    (void)cam;
    (void)camHand;

    float winW = 320.f * UIB_ContentSca;
    float winH = 500.f * UIB_ContentSca;

    ImGui::SetNextWindowPos({4, 4}, ImGuiCond_Once);
    ImGui::SetNextWindowSize({winW, winH}, ImGuiCond_Once);
    if (ImGui::Begin("Train", &msTrain->mShowWindow, 0)) draw_TrainUI();

    ImGui::End();

    ImGui::SetNextWindowPos({4 + winW, 4}, ImGuiCond_Once);
    ImGui::SetNextWindowSize({winW, winH}, ImGuiCond_Once);
    if (ImGui::Begin("Test", &mTest.mShowWindow, 0)) mTest.DrawTestUI();

    ImGui::End();
}
