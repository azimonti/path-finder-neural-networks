#ifndef _CS_SIM_H_9BCCA7480D8841D2B4C6221FA971E7B2_
#define _CS_SIM_H_9BCCA7480D8841D2B4C6221FA971E7B2_

/************************/
/*       cs_sim.h       */
/*    Version 1.0       */
/*     2022/06/26       */
/************************/

#include <memory>
#include <vector>
#include "cs_brainbase.h"
#include "cs_rbody.h"
#include "cs_serialize_fwd.h"
#include "cs_types.h"

namespace ge
{
    class Scene;
    class Mesh;
} // namespace ge

class CS_Unit;
class CS_Terrain;

class CS_Sim
{
  public:
    static constexpr double WALL_HEIGHT = 0.5;

    struct Params
    {
        size_t mInitUnitsN = 10;
        CS_RBody::Vec3 mStartPos{0, 0, 0};
        CS_RBody::Vec3 mTargetPos{0, 0, 0};
        double mMaxTimeS{};

        friend void to_json(nlohmann::json& j, const Params& v);
        friend void from_json(const nlohmann::json& j, Params& v);
    } mPars;

    CS_Terrain& mTerrain;
    const CS_BrainBase& mBrain;

  public:
    CS_Sim(const Params& par, CS_Terrain& terr, const CS_BrainBase& brain, bool createDisp);
    ~CS_Sim();

    static double GetWallHeight_s() { return WALL_HEIGHT; }

    double GetAvgTotalCost() const;

    double GetCurSimTimeS() const { return mCurTimeS; }

    void AnimSim(double intervalS, bool doDraw);

    void SetCompleted() { mIsCompleted = true; }

    bool IsSimComplete() const { return mIsCompleted; }

    void AddMeshesToSceneSim(ge::Scene& scene) const;
    void OnPickedMeshSim(const ge::Mesh* pMesh);
    void DrawSimParamsUI();
    void DrawSimUI();

  private:
    void drawSimStatusUI();
    void drawSelectedUI();
    size_t countRunning() const;
    size_t countSuccess() const;
    size_t countFailed() const;
    size_t countMax() const;

  private:
    std::vector<std::unique_ptr<CS_Unit>> moUnits;

    double mCurTimeS{};
    bool mIsCompleted{};

    CS_Unit* mpCurSelUnit{};
};

#endif
