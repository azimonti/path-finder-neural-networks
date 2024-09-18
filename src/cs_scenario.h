#ifndef _CS_SCENARIO_H_3DBE6817C7B3438FA985B855CA63CBB3_
#define _CS_SCENARIO_H_3DBE6817C7B3438FA985B855CA63CBB3_

/************************/
/*     main.cpp         */
/*    Version 1.0       */
/*     2022/06/24       */
/************************/

#include <memory>
#include <string>
#include <vector>
#include "camhandler.h"
#include "cs_scenarioterrsetup.h"
#include "cs_scenariotrain.h"
#include "cs_serialize_fwd.h"
#include "cs_sim.h"
#include "cs_terrain.h"

namespace ge
{
    class Scene;
    class Mesh;
} // namespace ge

class CS_BrainBase;
class CS_Terrain;
class CS_Trainer;
class CS_Player;

class CS_ScenarioTest
{
  public:
    CS_ScenarioTerrSetup mTerrSetup;
    std::unique_ptr<CS_Terrain> moTerr;
    std::unique_ptr<CS_Player> moPlayer;
    double mSpeedFactor{20};

    bool mShowWindow{true};

    void AnimateSceTest(bool doDraw);

    void DrawTestUI();
};

class CS_Scenario
{
  public:
    size_t mInitUnitsN = 1;
    std::unique_ptr<CS_Sim> moCurSim;
    std::unique_ptr<CS_BrainBase> moCurBrain;

    size_t mCurModelIdx = 0;

    std::shared_ptr<CS_ScenarioTrain> msTrain;
    CS_ScenarioTest mTest;

    CS_Chromo mPlayerChromo;

    std::vector<CS_Chromo> mBestChromos;
    std::vector<CS_ChromoInfo> mBestCInfos;

    double mWriteConfigTimeS = 0;

  public:
    CS_Scenario();
    ~CS_Scenario();

    void AnimScenario(bool doDraw);
    void AddMeshesToScene(ge::Scene& scene);
    void OnPickedMeshSce(const ge::Mesh* pMesh);
    void DrawScenarioUI(ge::Camera& cam, CH::CamHandMouse& camHand);

  private:
    void reqWriteConfig();
    void readConfig();
    void writeConfig() const;
    void stopTesting();
    void draw_TrainUI();
};

#endif
