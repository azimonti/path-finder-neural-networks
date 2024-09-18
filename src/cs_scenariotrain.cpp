#include <stdarg.h>
#include "log/log.h"
#include "cs_scenariotrain.h"
#include "cs_serialize.h"
#include "cs_terrain.h"
#include "cs_trainer.h"
#include "cs_utils.h"
#include "utils.h"

void CS_ScenarioTrain::localLog(const char* ftm, ...)
{
    char buffer[2048]{};
    va_list args;
    va_start(args, ftm);
    vsnprintf(buffer, sizeof(buffer), ftm, args);
    va_end(args);
    LOGGER(logging::INFO) << buffer;
}

void to_json(nlohmann::json& j, const CS_ScenarioTrain::Setup& v)
{
    j = nlohmann::json{
        CS_SERIALIZE_VAL(mTerrSetup),
    };
}

void from_json(const nlohmann::json& j, CS_ScenarioTrain::Setup& v)
{
    CS_DESERIALIZE_VAL(mTerrSetup);
}

CS_ScenarioTrain::CS_ScenarioTrain(const Setup& setup)
{
    mTerrSetup = setup.mTerrSetup;
}

CS_ScenarioTrain::~CS_ScenarioTrain()
{
    while (moTrainer)
    {
        moTrainer->ReqShutdown();

        auto& fut = moTrainer->GetTrainerFuture();
        // if the future is valid and it's ready
        if (fut.valid() && fut.wait_for(std::chrono::seconds(1)) == std::future_status::ready) moTrainer.reset();
    }
}

void CS_ScenarioTrain::AnimateSceTrain()
{
    if (!moTrainer) return;

    if (const auto curEpoch = moTrainer->GetCurEpochN(); curEpoch != mLastEpoch)
    {
        const auto curTimeS = ut::GetSteadyTimeS();
        mLastEpochLenTimeS  = curTimeS - mLastEpochTimeS;
        mLastEpoch          = curEpoch;
        mLastEpochTimeS     = curTimeS;
    }

    auto& fut = moTrainer->GetTrainerFuture();
    // if the future is valid and it's ready
    if (fut.valid() && fut.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        moTrainer->LockViewBestChromos([&](const auto&, const auto& infos) {
            if (infos.empty()) localLog("Training ended.");
            else
                localLog("Training ended. Best chromo: %s, cost:%s", infos.front().MakeStrID().c_str(),
                         CS_MakeCostString(infos.front().ci_cost).c_str());
        });

        moTrainer.reset();
    }
}
