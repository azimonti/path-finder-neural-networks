#ifndef _CS_UNIT_H_626BBBD4FE13402AAEBF69D0A7ED62F6_
#define _CS_UNIT_H_626BBBD4FE13402AAEBF69D0A7ED62F6_

/************************/
/*      cs_unit.h       */
/*    Version 1.0       */
/*     2022/06/26       */
/************************/

#include <memory>
#include <string>
#include <vector>
#include "cs_math.h"
#include "cs_rbody.h"
#include "cs_types.h"
#include "statetask.h"

namespace ge
{
    class Scene;
    class Mesh;
} // namespace ge

using CS_Pos      = glm::vec3;
using CS_UnitType = std::string;

class CS_UnitDisp
{
  public:
    std::vector<std::unique_ptr<ge::Mesh>> moRendMeshes;
    std::vector<ge::Transform> mRendMeshesXFs;

    CS_UnitDisp();
    void UpdateXForm(const CS_RBody& rbody);
};

class CS_Unit
{
    friend class CS_Sim;

  public:
    // assume the values are 0..1 and in seconds, meters, radians units
    // see CS_CTRL_MIN_VAL and CS_CTRL_MAX_VAL
    float mControls[CS_CTRL_N]{};

  private:
    double mLifeTimeS{};

    // momentary impulse force to consume in the next anim step
    CS_RBody::Vec3 mImpForcesLS{0, 0, 0};
    CS_RBody::Vec3 mImpTorquesLS{0, 0, 0};

    CS_RBody mRBody;

    static constexpr size_t POS_HIST_INTERVAL_S = 10;
    std::array<CS_RBody::Vec3, 60 * 2 / POS_HIST_INTERVAL_S> mPosHistory{};

    CS_UnitType mUnitType{};
    size_t mUnitID{};
    int mRunningState{}; // -1 failed, 0 running, 1 success
    // float           mDeadBendDir    {};

    double mFinalCost{};

    StateTask mState_Death;

    std::unique_ptr<CS_UnitDisp> moDisp;

  public:
    CS_Unit(const CS_UnitType& type, size_t id, const CS_Pos& pos, bool createDisp);
    ~CS_Unit();

    template <typename VEC_T> void SetControlValues(const VEC_T& controls)
    {
        assert(controls.size() == CS_CTRL_N);
        for (size_t i = 0; i < CS_CTRL_N; ++i)
            mControls[i] = std::clamp((float)controls[i], CS_CTRL_MIN_VAL, CS_CTRL_MAX_VAL);
    }

    auto GetControlValue(CS_ControlType type) const { return mControls[type]; }

    const auto& GetRBody() const { return mRBody; }

    auto& GetRBody() { return mRBody; }

    auto* GetDisp() const { return moDisp.get(); }

    bool IsNotMoving() const;

    void AddImpForceLS(const glm::dvec3& forceLS) { mImpForcesLS += forceLS; }

    void AddImpTorqueLS(const glm::dvec3& torqueLS) { mImpTorquesLS += torqueLS; }

    void AnimateUnit(double curTimeS, double intervalS);

    void SetRunningState(int state) { mRunningState = state; }

    auto GetRunningState() const { return mRunningState; }

    std::string GetRunningStateStr() const
    {
        switch (mRunningState)
        {
        case -1: return "Failed";
        case 0: return "Running";
        case 1: return "Success";
        }
        return "UNKNOWN";
    }

  private:
    void animate_ApplyControls(double intervalS);
};

#endif
