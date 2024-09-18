/************************/
/*     cs_unit.cpp      */
/*    Version 1.0       */
/*     2022/06/26       */
/************************/

#include "cs_unit.h"
#include "cs_utils.h"
#include "ge_mesh2.h"
#include "ge_mesh3.h"
#include "mesh.h"
#include "scene.h"

using Scalar                            = CS_RBody::Scalar;

static const auto MAX_SPEED_KPH         = (Scalar)20.0;                              // k/h
static const auto MAX_SPEED_MS          = (Scalar)(MAX_SPEED_KPH * 1000.0 / 3600.0); // m/s
static const auto MAX_FACCEL_MS2        = (Scalar)5.0;                               // m/s^2
static const auto MAX_BACCEL_MS2        = (Scalar)1.5;                               // m/s^2
static const auto MAX_BRAKE_COE         = (Scalar)0.1;             // coefficient of braking respect to the current acc
static const auto MAX_STEER_RAD_S       = (Scalar)ge::ToRad(45.0); // rad/s
// speed at which we already reach the maximum ability to turn
static const auto SPEED_OF_MAX_STEER_MS = (Scalar)(MAX_SPEED_MS / 8.0);
static const auto NOTMOVING_DIST_M      = (Scalar)0.5;

CS_UnitDisp::CS_UnitDisp()
{
    const float H = 1.2f;
    moRendMeshes.push_back(
        gegt::MakeCuboidMesh(false, glm::vec3(0.50f, H, 0.25f), glm::vec3(0, H / 2, 0), ge::Color(1, 0, 0, 1)));
    mRendMeshesXFs.push_back(glm::mat4(1));

    const float H2 = 0.7f;
    moRendMeshes.push_back(gegt::MakeCuboidMesh(false, glm::vec3(0.50f, H2, 0.90f), glm::vec3(0, H2 / 2, -0.90f / 2),
                                                ge::Color(0, 0, 1, 1)));
    mRendMeshesXFs.push_back(glm::mat4(1));
}

void CS_UnitDisp::UpdateXForm(const CS_RBody& rbody)
{
    // update the transform
    for (auto& moRendMesh : moRendMeshes)
    {
        auto& xf = moRendMesh->GetTransform();
        xf       = {};
        // get the position from the rigid body simulation
        xf.Translate(rbody.GetPosWS());
        xf.RotateByMat(rbody.GetRotWS_LS());
    }
}

CS_Unit::CS_Unit(const CS_UnitType& type, size_t id, const CS_Pos& pos, bool createDisp) : mUnitType(type), mUnitID(id)
{
    mRBody.mPosWS = pos;

    if (createDisp) moDisp = std::make_unique<CS_UnitDisp>();
}

CS_Unit::~CS_Unit() = default;

void CS_Unit::animate_ApplyControls(double intervalS)
{
    // convert input to impulse forces
    if (const auto unit = (CS_RBody::Scalar)mControls[CS_CTRL_FACCEL]) // acceleration
    {
        const auto valMS2  = unit * MAX_FACCEL_MS2;
        const auto forceLS = CS_RBody::Vec3{0, 0, -valMS2} * mRBody.mMass;
        AddImpForceLS(forceLS);
    }
    if (const auto unit = (CS_RBody::Scalar)mControls[CS_CTRL_BACCEL]) // back acceleration
    {
        const auto valMS2  = unit * MAX_BACCEL_MS2;
        const auto forceLS = CS_RBody::Vec3{0, 0, valMS2} * mRBody.mMass;
        AddImpForceLS(forceLS);
    }
    if (const auto unit = (CS_RBody::Scalar)mControls[CS_CTRL_BRAKE]) // braking
    {
        // for braking, first we convert the current rigid body WS acceleration to LS
        // const auto curAccLS = mRBody.CalcRotLS_WS() * mRBody.GetAccWS();
        const auto curVelLS   = mRBody.CalcRotLS_WS() * mRBody.GetVelWS();
        // then we attenuate the braking force by the current acceleration
        // const auto brakeAccLS = curAccLS * (unit * MAX_BRAKE_COE);
        const auto brakeVelLS = curVelLS * (unit * MAX_BRAKE_COE) * (Scalar)-1;
        const auto brakeAccLS = brakeVelLS / (Scalar)intervalS;
        const auto forceLS    = brakeAccLS * mRBody.mMass;
        AddImpForceLS(forceLS);
    }

    // steering
    if (const auto unit = (CS_RBody::Scalar)(mControls[CS_CTRL_STEER_L] - mControls[CS_CTRL_STEER_R]))
    {
        const auto curVelLS = mRBody.CalcRotLS_WS() * mRBody.GetVelWS();
        // quick conversion from speed to steering radius
        const auto speedCoe = std::min((Scalar)1.0, -curVelLS[2] / SPEED_OF_MAX_STEER_MS);
        const auto valRadS  = unit * MAX_STEER_RAD_S * speedCoe * intervalS;
        mRBody.mAngVelLS[1] = (Scalar)valRadS;
    }
}

void CS_Unit::AnimateUnit(double curTimeS, double intervalS)
{
    mState_Death.AnimStateTask(curTimeS);

    animate_ApplyControls(intervalS);

    // iterate the rigid body simulation with optional new forces added
    mRBody.StepSimulation((Scalar)intervalS, mImpForcesLS, mImpTorquesLS);
    mImpForcesLS     = {0, 0, 0}; // reset impulse force after being consumed
    mImpTorquesLS    = {0, 0, 0}; // reset impulse torque after being consumed

    const auto speed = std::max((Scalar)1.0, glm::length(mRBody.GetVelWS()));

    // standard attentuation / hard limit
    mRBody.AttenuateVel((Scalar)intervalS, (Scalar)(speed < MAX_SPEED_MS ? 0.0 : 0.5));

    // we reset the angular velocity every time, to simulate just the steering
    mRBody.AttenuateAngVel((Scalar)intervalS, (Scalar)(1.0 / intervalS));

    // update the lifetime
    mLifeTimeS += intervalS;
    // update the velocity history
    const auto postHistIdx                            = (((size_t)mLifeTimeS) / POS_HIST_INTERVAL_S);
    mPosHistory[postHistIdx % std::size(mPosHistory)] = mRBody.GetPosWS();
}

bool CS_Unit::IsNotMoving() const
{
    // make sure we have plenty of samples
    const auto postHistIdx = (((size_t)mLifeTimeS) / POS_HIST_INTERVAL_S);
    if (postHistIdx < std::size(mPosHistory)) return false;

    auto mi = mPosHistory[0];
    auto ma = mPosHistory[0];
    for (const auto& pos : mPosHistory)
    {
        mi = glm::min(mi, pos);
        ma = glm::max(ma, pos);
    }

    const auto diff = ma - mi;
    return diff[0] < NOTMOVING_DIST_M && diff[1] < NOTMOVING_DIST_M && diff[2] < NOTMOVING_DIST_M;
}
