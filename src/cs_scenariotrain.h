#ifndef CS_SCENARIOTRAIN_H
#define CS_SCENARIOTRAIN_H

#include "cs_scenarioterrsetup.h"
#include "cs_serialize_fwd.h"
#include "cs_trainbase.h"
#include "cs_sim.h"

class CS_Terrain;
class CS_Trainer;

class CS_ScenarioTrain
{
  public:
    struct Setup
    {
        CS_ScenarioTerrSetup mTerrSetup;

        friend void to_json(nlohmann::json& j, const Setup& v);
        friend void from_json(const nlohmann::json& j, Setup& v);
    };

    CS_ScenarioTerrSetup mTerrSetup;
    std::vector<std::unique_ptr<CS_Terrain>> moTerrs;
    std::vector<CS_Sim::Params> mSimPars;
    std::unique_ptr<CS_Trainer> moTrainer;

    size_t mLastEpoch         = 0;
    double mLastEpochTimeS    = 0;
    double mLastEpochLenTimeS = 0;

    bool mShowWindow{true};

    CS_ScenarioTrain() = default;
    CS_ScenarioTrain(const Setup& setup);
    ~CS_ScenarioTrain();

    Setup MakeSetup() const
    {
        Setup setup;
        setup.mTerrSetup = mTerrSetup;
        return setup;
    }

    void AnimateSceTrain();

  private:
    static void localLog(const char* ftm, ...);
};

#endif
