/************************/
/*      cs_sim.cpp      */
/*    Version 1.0       */
/*     2022/06/26       */
/************************/

#include <algorithm>
#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <glm/gtx/string_cast.hpp>
#pragma GCC diagnostic pop
#else
#include <glm/gtx/string_cast.hpp>
#endif
#include <glm/glm.hpp>
#include "cs_math.h"
#include "cs_serialize.h"
#include "cs_sim.h"
#include "cs_terrain.h"
#include "cs_unit.h"
#include "cs_utils.h"
#include "implot.h"
#include "mesh.h"
#include "scene.h"
#include "uibase.h"
#include "utils.h"

namespace glm
{

    void to_json(nlohmann::json& j, const vec3& v)
    {
        j = nlohmann::json{
            {"x", v.x},
            {"y", v.y},
            {"z", v.z}
        };
    }

    void to_json(nlohmann::json& j, const dvec3& v)
    {
        j = nlohmann::json{
            {"x", v.x},
            {"y", v.y},
            {"z", v.z}
        };
    }

    void from_json(const nlohmann::json& j, vec3& v)
    {
        j.at("x").get_to(v.x);
        j.at("y").get_to(v.y);
        j.at("z").get_to(v.z);
    }

    void from_json(const nlohmann::json& j, dvec3& v)
    {
        j.at("x").get_to(v.x);
        j.at("y").get_to(v.y);
        j.at("z").get_to(v.z);
    }

} // namespace glm

void to_json(nlohmann::json& j, const CS_Sim::Params& v)
{
    j = nlohmann::json{
        // CS_SERIALIZE_VAL(mInitUnitsN),
        CS_SERIALIZE_VAL(mStartPos),
        CS_SERIALIZE_VAL(mTargetPos),
        CS_SERIALIZE_VAL(mMaxTimeS),
    };
}

void from_json(const nlohmann::json& j, CS_Sim::Params& v)
{
    // CS_DESERIALIZE_VAL(mInitUnitsN);
    CS_DESERIALIZE_VAL(mStartPos);
    CS_DESERIALIZE_VAL(mTargetPos);
    CS_DESERIALIZE_VAL(mMaxTimeS);
}

CS_Sim::CS_Sim(const Params& par, CS_Terrain& terr, const CS_BrainBase& brain, bool createDisp)
    : mPars(par), mTerrain(terr), mBrain(brain)
{
    const float distX = mTerrain.GetCellSize() * 8.0f;
    const float distZ = mTerrain.GetCellSize() * 8.0f;

    const auto n      = mPars.mInitUnitsN;

    const auto sideN  = std::max((size_t)1, (size_t)ceil(sqrt((double)n)));

    const auto colsN  = sideN;
    const auto rowsN  = sideN;

    for (size_t row = 0; row < rowsN; ++row)
    {
        for (size_t col = 0; col < colsN; ++col)
        {
            // make a progressive grid starting from the center
            const auto ix   = (int)col;
            const auto iz   = (int)row;
            // signed from center
            const auto sx   = ((ix & 1) * 2 - 1) * (ix / 2);
            const auto sz   = ((iz & 1) * 2 - 1) * (iz / 2);
            const auto offX = (float)sx * distX;
            const auto offZ = (float)sz * distZ;
            const float x   = (float)mPars.mStartPos[0] + offX;
            const float z   = (float)mPars.mStartPos[2] + offZ;
            const auto pos  = glm::vec3(x, 0, z);

            // skip bad positions
            if (!mTerrain.IsPosInside(pos)) continue;
            if (mTerrain.GetHeightFromPos(pos) >= WALL_HEIGHT) continue;

            // for now the ID of the unit is just a counter
            const auto unitID = moUnits.size();
            // create the unit
            moUnits.push_back(std::make_unique<CS_Unit>("car", unitID, pos, createDisp));

            if (moUnits.size() >= n) break;
        }
        if (moUnits.size() >= n) break;
    }

    // start the selection with the first unit
    mpCurSelUnit = moUnits[0].get();
}

CS_Sim::~CS_Sim() = default;

#if 0
inline double MakeCleanAngle(double ang)
{
    const auto PI2 = glm::two_pi<double>();
    return ang - PI2 * std::floor( ang / PI2 );
}

inline auto makeSteerAngle = [](double angle)
{
    const auto PI = glm::pi<double>();
    const auto PI2 = glm::two_pi<double>();
    if (angle >  PI) angle -= PI2; else
    if (angle < -PI) angle += PI2;
    return angle;
};
#endif
inline auto getFwdVecNorm              = [](const auto& rot) { return -glm::normalize(rot[2]); };

static auto calcYaw                    = [](const auto& fwd) { return (decltype(fwd[0]))atan2(fwd[0], fwd[2]); };

[[maybe_unused]] inline auto appendYaw = [](const auto& rotWS_LS, auto yawLS) {
    using Type      = decltype(yawLS);
    const auto rotY = glm::rotate(glm::mat<4, 4, Type, glm::defaultp>(1), yawLS, {0, 1, 0});
    return rotWS_LS * glm::mat<3, 3, Type, glm::defaultp>(rotY);
};

// basic unit brain... stock logic, no machine learning
static void prepareBrainInputs(CSM_Vec& inputs, const CS_Unit& u, const glm::dvec3& targetPos,
                               const CS_Terrain& terrain,
                               const std::function<void(const glm::vec3&, const glm::vec4&)>& drawDebugDotFn)
{
    const auto& rb             = u.GetRBody();

    inputs[CS_SENS_TARGET_X]   = (CS_SCALAR)targetPos[0];
    inputs[CS_SENS_TARGET_Z]   = (CS_SCALAR)targetPos[2];
    inputs[CS_SENS_POS_X]      = (CS_SCALAR)rb.mPosWS[0];
    inputs[CS_SENS_POS_Z]      = (CS_SCALAR)rb.mPosWS[2];
    const auto fwdVec          = getFwdVecNorm(rb.GetRotWS_LS());
    inputs[CS_SENS_FWD_X]      = (CS_SCALAR)fwdVec[0];
    inputs[CS_SENS_FWD_Z]      = (CS_SCALAR)fwdVec[2];
    inputs[CS_SENS_VEL_X]      = (CS_SCALAR)rb.GetVelWS()[0];
    inputs[CS_SENS_VEL_Z]      = (CS_SCALAR)rb.GetVelWS()[2];

    // velocity-based sensors
    const auto speed           = (float)glm::length(rb.GetVelWS());
    const auto probeUnit       = std::max(speed * speed * 0.2f, 10.0f);

    inputs[CS_SENS_PROBE_UNIT] = (CS_SCALAR)probeUnit;

    const auto WALL_HEIGHT     = 0.5f;

    struct Probe
    {
        glm::vec3 pr_offsetLS{};
        glm::vec4 pr_debugCol{};
        float pr_hitDist = 0;
    };

    Probe probes[CS_SENS_PROBES_N];
#if 1
    // setup the end position for the probes for the ray scan
    probes[0].pr_offsetLS = {0 * probeUnit, 0, -2 * probeUnit};                     // center
    probes[1].pr_offsetLS = {-1 * probeUnit, 0, -1 * probeUnit};                    // left
    probes[2].pr_offsetLS = {1 * probeUnit, 0, -1 * probeUnit};                     // right
    probes[3].pr_offsetLS = (probes[0].pr_offsetLS + probes[1].pr_offsetLS) * 0.5f; // center-left
    probes[4].pr_offsetLS = (probes[0].pr_offsetLS + probes[2].pr_offsetLS) * 0.5f; // center-right
    probes[5].pr_offsetLS = (probes[0].pr_offsetLS + probes[3].pr_offsetLS) * 0.5f; // left-right
    probes[6].pr_offsetLS = (probes[0].pr_offsetLS + probes[4].pr_offsetLS) * 0.5f; // left-right

    probes[0].pr_debugCol = {1, 0, 0, 1};
    probes[1].pr_debugCol = {0, 1, 0, 1};
    probes[2].pr_debugCol = {0, 0, 1, 1};
    probes[3].pr_debugCol = (probes[0].pr_debugCol + probes[1].pr_debugCol) * 0.5f;
    probes[4].pr_debugCol = (probes[0].pr_debugCol + probes[2].pr_debugCol) * 0.5f;
    probes[5].pr_debugCol = (probes[0].pr_debugCol + probes[3].pr_debugCol) * 0.5f;
    probes[6].pr_debugCol = (probes[0].pr_debugCol + probes[4].pr_debugCol) * 0.5f;
#else
    static glm::vec4 primaryCols[CS_SENS_PROBES_N] = {
        {1.0f, 0.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 1.0f, 1.0f},
        {1.0f, 0.0f, 1.0f, 1.0f},
        {1.0f, 0.5f, 0.0f, 1.0f},
        {0.0f, 1.0f, 0.5f, 1.0f},
        {0.5f, 0.0f, 1.0f, 1.0f},
        {1.0f, 0.0f, 0.5f, 1.0f},
        {0.5f, 1.0f, 0.0f, 1.0f},
        {0.0f, 0.5f, 1.0f, 1.0f},
        {1.0f, 0.5f, 0.5f, 1.0f},
        {0.5f, 1.0f, 0.5f, 1.0f},
        {0.5f, 0.5f, 1.0f, 1.0f},
    };
    for (int i = 0; i < CS_SENS_PROBES_N; ++i)
    {
        const auto ang        = glm::two_pi<float>() * i / (float)CS_SENS_PROBES_N;
        probes[i].pr_offsetLS = {probeUnit * std::cos(ang), 0, probeUnit * std::sin(ang)};
        probes[i].pr_debugCol = primaryCols[i];
    }
#endif
    const auto maxDistance = terrain.GetFieldSize();

    // initialize to a large reference value
    for (auto& probe : probes) probe.pr_hitDist = maxDistance;

    // do the ray scan
    for (auto& probe : probes)
    {
        terrain.ScanRay(rb.mPosWS, rb.mPosWS + rb.GetRotWS_LS() * probe.pr_offsetLS,
                        [&](const auto& pos, const auto h, bool isOutsideMap) {
            // terrain.DI_DrawCellAtPos(pos, {0.7f,0,0,1}, {0,0,0,0.5f});
            if (probe.pr_debugCol[3] && drawDebugDotFn) drawDebugDotFn(pos, probe.pr_debugCol);

            if (h > WALL_HEIGHT || isOutsideMap)
            {
                probe.pr_hitDist = glm::length(pos - glm::vec3(rb.mPosWS));
                return false;
            }
            return true;
        });
    }

    for (size_t i = 0; i < (size_t)CS_SENS_PROBES_N; ++i)
        inputs[(size_t)CS_SENS_PROBE_FIRST_HITDIST + i] = probes[i].pr_hitDist;

    // normalize to our reference value
    for (auto& in : inputs) in /= maxDistance;

    // we do not normalize these, since they are flags, not distances
    inputs[CS_SENS_IS_OUTSIDE_MAP]  = (CS_SCALAR)(terrain.IsPosInside(rb.mPosWS) ? 0 : 1);
    inputs[CS_SENS_IS_IN_DEAD_ZONE] = (CS_SCALAR)(terrain.GetHeightFromPos(rb.mPosWS) > WALL_HEIGHT ? 1 : 0);
}

// calculate a cost function based on the current state
static double calcCost(const CSM_Vec& inputs, double curTimeS, double maxTimeS)
{
    constexpr double BAD_AREA_FACTOR = 10.0; // >> of sum of other factors

    const auto targetPos             = glm::dvec3(inputs[CS_SENS_TARGET_X], 0, inputs[CS_SENS_TARGET_Z]);
    const auto pos                   = glm::dvec3(inputs[CS_SENS_POS_X], 0, inputs[CS_SENS_POS_Z]);
    const auto dist                  = glm::length(targetPos - pos);

    const auto isOutsideMap          = inputs[CS_SENS_IS_OUTSIDE_MAP] > 0.5;
    const auto isInDeadZone          = inputs[CS_SENS_IS_IN_DEAD_ZONE] > 0.5;

    return dist + isOutsideMap * BAD_AREA_FACTOR + isInDeadZone * BAD_AREA_FACTOR + curTimeS / maxTimeS;
}

// calculate a cost function based on the current state
[[maybe_unused]] static double hasCrashed(const CSM_Vec& inputs)
{
    return inputs[CS_SENS_IS_OUTSIDE_MAP] > 0.5 || inputs[CS_SENS_IS_IN_DEAD_ZONE] > 0.5;
}

double CS_Sim::GetAvgTotalCost() const
{
    double totalCost = 0.0;
    for (const auto& u : moUnits) totalCost += u->mFinalCost;

    return totalCost / std::max(1.0, (double)moUnits.size());
}

void CS_Sim::AnimSim(double intervalS, bool doDraw)
{
    // update the completed status
    mIsCompleted = (countRunning() == 0) || (mCurTimeS >= mPars.mMaxTimeS);

    // update the absolute timer
    mCurTimeS += intervalS;

    std::function<void(const glm::vec3&, const glm::vec4&)> drawDebugDot;
    if (doDraw)
    {
        // highlight the selected unit, if any
        const auto liveTimeS = ut::GetSteadyTimeS();
        const auto selColSca = (float)(sin(liveTimeS * 7) + 2.0);
        const auto baseCol   = ge::Vec3{1.f, 1.f, 1.f};
        for (const auto& u : moUnits)
        {
            const auto isSel = (u.get() == mpCurSelUnit);
            for (auto& mesh : u->GetDisp()->moRendMeshes)
                mesh->GetMaterial().mDiffuseCol = isSel ? baseCol * selColSca : baseCol;
        }

        drawDebugDot = [&](const auto& pos, const auto& col) { mTerrain.DI_DrawCellAtPos(pos, col, {0, 0, 0, 0.0f}); };
    }

    // nothing else to do if the simulation is completed
    if (mIsCompleted) return;

    // preallocate inputs and outputs
    CS_SCALAR inputsBuff[CS_SENS_N];
    CS_SCALAR outputsBuff[CS_CTRL_N];
    CSM_Vec inputs(inputsBuff, CS_SENS_N);   // UNINITIALIZED
    CSM_Vec outputs(outputsBuff, CS_CTRL_N); // UNINITIALIZED

    auto onUnitEnd = [&](const auto& u, const auto& inputs, bool assumeTimeout, int state) {
        u->SetRunningState(state);
        const auto useTimeS = assumeTimeout ? mPars.mMaxTimeS : mCurTimeS;
        u->mFinalCost       = calcCost(inputs, useTimeS, mPars.mMaxTimeS);
    };

    // animate all of the units
    for (size_t i = 0; i < moUnits.size(); ++i)
    {
        const auto& u = moUnits[i];

        // always start with zeros in inputs and outputs
        inputs.ZeroFill();
        outputs.ZeroFill();

        // setup the input variables for the brain
        prepareBrainInputs(inputs, *u, mPars.mTargetPos, mTerrain, drawDebugDot);

        if (u->GetRunningState() != 0) continue;

        // we know this from here
        if (hasCrashed(inputs))
        {
            onUnitEnd(u, inputs, false, -1);
            continue;
        }
        if (u->IsNotMoving())
        {
            onUnitEnd(u, inputs, true, -1);
            continue;
        }

        // execute the default brain
        mBrain.AnimateBrain(inputs, outputs);

        // apply the brain outputs as inputs to the unit
        u->SetControlValues(outputs);

        // run the simulation step
        u->AnimateUnit(mCurTimeS, intervalS);

        // we know this right after the simulation step
        {
            const auto distoToTarget = glm::distance(u->GetRBody().mPosWS, mPars.mTargetPos);
            if (distoToTarget < 1.0) onUnitEnd(u, inputs, false, 1);               // success
            else if (mCurTimeS > mPars.mMaxTimeS) onUnitEnd(u, inputs, false, -1); // fail
        }
    }
}

size_t CS_Sim::countRunning() const
{
    size_t cnt{};
    for (const auto& u : moUnits) cnt += u->GetRunningState() == 0;

    return cnt;
}

size_t CS_Sim::countSuccess() const
{
    size_t cnt{};
    for (const auto& u : moUnits) cnt += u->GetRunningState() > 0;

    return cnt;
}

size_t CS_Sim::countFailed() const
{
    size_t cnt{};
    for (const auto& u : moUnits) cnt += u->GetRunningState() < 0;

    return cnt;
}

size_t CS_Sim::countMax() const
{
    return moUnits.size();
}

void CS_Sim::AddMeshesToSceneSim(ge::Scene& scene) const
{
    for (auto& u : moUnits)
    {
        // do we have a mesh ?
        auto* pDisp = u->GetDisp();
        pDisp->UpdateXForm(u->GetRBody());
        for (auto& oMesh : pDisp->moRendMeshes)
        {
            // add the mesh tot he 3D scene for rendering
            scene.AddMesh(*oMesh);

            mTerrain.DI_DrawCellAtPos(u->GetRBody().mPosWS, {0, 0.7f, 0, 1}, {0, 0, 0.7f, 1});
        }
    }
}

void CS_Sim::OnPickedMeshSim(const ge::Mesh* pMesh)
{
    mpCurSelUnit = nullptr;
    for (auto& u : moUnits)
    {
        // do we have a mesh ?
        for (auto& oMesh : u->GetDisp()->moRendMeshes)
        {
            if (oMesh.get() == pMesh)
            {
                mpCurSelUnit = u.get();
                return;
            }
        }
    }
}

void CS_Sim::drawSimStatusUI()
{
    ImGui::Text("Status:");
    ImGui::SameLine();
    ImGui::TextColored(!IsSimComplete() ? UIB_COL_GREEN : UIB_COL_RED, !IsSimComplete() ? "Running..." : "Complete");

    ImGui::Text("Cur Time (S): %.1f", mCurTimeS);

    if (ImGui::BeginTable("raceStats", (int)2))
    {
        ImGui::TableSetupColumn("");
        // ImGui::TableNextColumn();
        ImGui::TableSetupColumn(CS_MakeRaceName(0).c_str());

        ImGui::TableHeadersRow();

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("%s", "Target Pos");
        ImGui::TableNextColumn();
        const auto& tpos = mPars.mTargetPos;
        ImGui::Text("%.1f %.1f %.1f", tpos[0], tpos[0], tpos[2]);

        auto makeCntEntry = [&](const char* pName, const auto& calcFn) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%s", pName);
            ImGui::TableNextColumn();
            ImGui::Text("%zu", calcFn());
        };
        makeCntEntry("Total", [this]() { return moUnits.size(); });
        makeCntEntry("Running", [this]() { return countRunning(); });
        makeCntEntry("Success", [this]() { return countSuccess(); });
        makeCntEntry("Failed", [this]() { return countFailed(); });

        ImGui::EndTable();
    }
}

template <typename VEC_T> static double calcYawToTarget(const VEC_T& fwd, const VEC_T& pos, const VEC_T& targetPos)
{
    const auto targetDir = glm::normalize(targetPos - pos);
    // const auto fwdXZ = glm::normalize( VEC_T(fwd[0], 0.0f, fwd[2]) );
    double yaw           = atan2(targetDir[2], targetDir[0]) - atan2(fwd[2], fwd[0]);
    yaw                  = atan2(sin(yaw), cos(yaw));
    return yaw;
}

void CS_Sim::drawSelectedUI()
{
    if (!ImGui::BeginTable("selUnitProps", 2)) return;

    UIB_EasyTable et({"Property", "Value"});

    const auto& u = *mpCurSelUnit;
    et.AddText("Race");
    et.AddText(CS_MakeRaceName(0));
    et.AddText("ID");
    et.AddText(std::to_string(u.mUnitID));
    et.AddText("Type");
    et.AddText(u.mUnitType);
    et.AddText("Status");
    switch (u.mRunningState)
    {
    case -1: et.SetNextColor(UIB_COL_RED); break;
    case 0: et.SetNextColor(UIB_COL_WHITE); break;
    case 1: et.SetNextColor(UIB_COL_GREEN); break;
    default: assert(0);
    }
    et.AddText(u.GetRunningStateStr());
    et.AddText("Final Cost");
    et.AddText(CS_MakeCostString(u.mFinalCost));

    const auto& rb         = u.GetRBody();
    const auto fwd         = getFwdVecNorm(rb.GetRotWS_LS());
    const auto curYaw      = calcYaw(fwd);
    const auto yawToTarget = calcYawToTarget(fwd, rb.mPosWS, mPars.mTargetPos);

    et.AddText("Pos");
    et.AddText(glm::to_string(rb.mPosWS));
    et.AddText("Yaw");
    et.AddText(std::to_string(glm::degrees(curYaw)));
    et.AddText("Yaw to Target");
    et.AddText(std::to_string(glm::degrees(yawToTarget)));

    for (int i = 0; i < CS_CTRL_N; ++i)
    {
        const auto type = (CS_ControlType)i;
        et.AddText(std::string("Control: ") + CS_GetControlName(type));
        et.AddText(std::to_string(u.GetControlValue(type)));
    }

    ImGui::EndTable();
}

inline auto makeWidget = [](const char* pName, const auto& val) {
    if (typeid(val) == typeid(glm::vec3)) ImGui::InputScalarN(pName, ImGuiDataType_Float, (void*)&val, 3);
    else if (typeid(val) == typeid(glm::dvec3)) ImGui::InputScalarN(pName, ImGuiDataType_Double, (void*)&val, 3);
    else if (typeid(val) == typeid(double)) ImGui::InputScalar(pName, ImGuiDataType_Double, (void*)&val);
    else { assert(0); }
};

void CS_Sim::DrawSimParamsUI()
{
    if (!UIB_Header("Simulation Parameters")) return;

    makeWidget("Start Pos", mPars.mStartPos);
    makeWidget("Target Pos", mPars.mTargetPos);
    makeWidget("Max Time (S)", mPars.mMaxTimeS);
}

void CS_Sim::DrawSimUI()
{
    if (UIB_Header("Simulation", true, true)) drawSimStatusUI();

    if (mpCurSelUnit && UIB_Header("Selected")) drawSelectedUI();
}
