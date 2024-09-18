#ifndef _CS_RBODY_H_
#define _CS_RBODY_H_

/************************/
/*      cs_unit.h       */
/*    Version 1.0       */
/*     2023/02/09       */
/************************/

#include <glm/ext/matrix_transform.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>
#include "cs_types.h"

inline auto IntegrateNewton = [](auto& pos, auto& vel, const auto& acc, auto dt) {
    auto oldVel = vel;
    vel += acc * dt;
    pos += (oldVel + vel) * static_cast<decltype(dt)>(0.5) * dt;
};

#if 0
template <typename T>
inline bool IsGoodReal( const T &v )
{
    return !std::isnan( v ) && !std::isinf( v );
}
#endif

template <typename T> class CS_RBodyT
{
  public:
    using Scalar = T;

    using Vec3   = glm::vec<3, T, glm::defaultp>;
    using Mat3   = glm::mat<3, 3, T, glm::defaultp>;
    using Mat4   = glm::mat<4, 4, T, glm::defaultp>;

  public:
    T mMass{1};
    Vec3 mPosWS{0, 0, 0};
    Vec3 mVelWS{0, 0, 0};
    Vec3 mAccWS{0, 0, 0};

    Vec3 mAngVelLS{0, 0, 0};
    Mat3 mCurRotWS_LS{1};

    const auto& GetPosWS() const { return mPosWS; }

    const auto& GetVelWS() const { return mVelWS; }

    const auto& GetAccWS() const { return mAccWS; }

    const auto& GetRotWS_LS() const { return mCurRotWS_LS; }

    auto CalcRotLS_WS() const { return glm::inverse(mCurRotWS_LS); }

    void StepSimulation(T dt, const Vec3& forcesLS, const Vec3& torqueLS)
    {
        (void)torqueLS;
        mAccWS = (mCurRotWS_LS * forcesLS) / mMass;
        IntegrateNewton(mPosWS, mVelWS, mAccWS, dt);

        // mAngVelLS += torqueLS * dt;
        auto tmp     = Mat4(mCurRotWS_LS);
        tmp          = glm::rotate(tmp, mAngVelLS[0], Vec3{1, 0, 0});
        tmp          = glm::rotate(tmp, mAngVelLS[1], Vec3{0, 1, 0});
        tmp          = glm::rotate(tmp, mAngVelLS[2], Vec3{0, 0, 1});
        mCurRotWS_LS = Mat3(tmp);
    }

    // just a quick way to simulate any kind of drag or friction
    template <typename S> void AttenuateVel(S dt, S att) { mVelWS *= (T)(1 - att * dt); }

    template <typename S> void AttenuateAngVel(S dt, S att) { mAngVelLS *= (T)(1 - att * dt); }

    template <typename S> void AttenuateAcc(S dt, S att) { mAccWS *= (T)(1 - att * dt); }
};

// using CS_RBody = CS_RBodyT<CS_SCALAR>;
using CS_RBody = CS_RBodyT<double>;

#endif
