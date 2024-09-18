#ifndef _MOUSECAMHANDLER_H_E1AD9A20618241D98D8AF088B6BE5113_
#define _MOUSECAMHANDLER_H_E1AD9A20618241D98D8AF088B6BE5113_

/************************/
/*    camhandler.h      */
/*    Version 1.0       */
/*     2022/06/29       */
/************************/

#include <algorithm>
#include <array>
#include <chrono>
#include <functional>
#include <unordered_map>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace CH
{

    using Vec2  = glm::vec2;
    using Vec3  = glm::vec3;
    using Vec4  = glm::vec4;
    using Rot_t = glm::mat3;
    using Xfo_t = glm::mat4;

    inline double GetSteadyTimeS()
    {
        return (double)std::chrono::duration_cast<std::chrono::microseconds>(
                   std::chrono::steady_clock::now().time_since_epoch())
                   .count() /
               1e6;
    }

    class TouchTask
    {
      public:
        double mBeginTimeS{};
        Vec2 mBeginPos{};
        double mMoveTimeS{};
        Vec2 mCurPos{};

        void OnTouchBegin(float x, float y);
        void OnTouchEnd(float x, float y);
        void OnTouchMove(float x, float y);

        bool IsTouchActive() const { return mBeginTimeS != 0.0; }

        Vec2 GetTouchOff() const { return mCurPos - mBeginPos; }
    };

    inline void TouchTask::OnTouchBegin(float x, float y)
    {
        mBeginTimeS = mMoveTimeS = GetSteadyTimeS();
        mBeginPos[0]             = x;
        mBeginPos[1]             = y;
    }

    inline void TouchTask::OnTouchEnd(float x, float y)
    {
        mBeginTimeS = 0;
        mMoveTimeS  = GetSteadyTimeS();
        mCurPos[0]  = x;
        mCurPos[1]  = y;
    }

    inline void TouchTask::OnTouchMove(float x, float y)
    {
        mMoveTimeS = GetSteadyTimeS();
        mCurPos[0] = x;
        mCurPos[1] = y;
    }

    class TDeltaTracker
    {
        double mLastTimeS{};

      public:
        double GetDeltaTimeS(double curTimeS = 0)
        {
            if (!curTimeS) curTimeS = GetSteadyTimeS();

            const auto d = mLastTimeS ? curTimeS - mLastTimeS : 0.0;

            mLastTimeS   = curTimeS;

            return d;
        }
    };

    inline auto getRotSca = [](const auto& xf) { return Rot_t(xf); };
    inline auto getTra    = [](const auto& xf) { return Vec3(xf[3]); };

    class CamHandMouse
    {
        static constexpr double PI_VAL = glm::pi<double>();

      public:
        enum Mode : int { MODE_HOOK, MODE_FLY };

        struct Limits
        {
            glm::dvec2 li_maxRadSec{PI_VAL / 180.0, PI_VAL / 90.0}; // pitch, yaw
        };

        struct Mappings
        {
            int km_arrowL{};
            int km_arrowR{};
            int km_arrowU{};
            int km_arrowD{};
        };

      private:
        static constexpr size_t MAX_BUTTONS = 4;

        Mappings mMappings;
        Limits mLimits;

        Mode mMode{MODE_HOOK};

        Vec3 mPick_PosWS{0, 0, 0};

        Rot_t mCamRot_StaRotCS_WS{1.f};
        Vec3 mCamRot_StaTraCS{};
        Vec3 mCamRot_StaTraWS{};

        Rot_t mCamPan_StaRot{1.f};
        Vec3 mCamPan_StaTraCS{};

        TouchTask mLTouchTask;
        TouchTask mRTouchTask;

        bool mPick_SkipOnce{};
        bool mPick_Active{};
        Vec2 mPick_Point{};

        bool mDClick_Active{};
        Vec2 mDClick_Point{};

        struct KeyState
        {
            bool ks_isDown{};
            bool ks_isAngle{};
            double ks_strength{};
            TDeltaTracker ks_tdelta;
        };

        std::unordered_map<int, KeyState> mKeyPressMap;

        TDeltaTracker mButtPressTDeltas[MAX_BUTTONS];
        TDeltaTracker mScrollTDelta;
        // double      mScrollSca              {1.0};

        TDeltaTracker mAnimTDelta;
        // double      mLastAnimTimeS          {};
        glm::dvec3 mAccCS{};
        glm::dvec3 mSpeedCS{};
        glm::dvec3 mRotXYAcc{};
        glm::dvec3 mRotXYSpeed{};

        // std::array<float,16>    mProjMtx    {};

      public:
        void SetMappings(const Mappings& maps) { mMappings = maps; }

        void SetCamMode(Mode mode) { mMode = mode; }

        Mode GetCamMode() const { return mMode; }

        bool GetPickMessage()
        {
            auto val     = mPick_Active;
            mPick_Active = false;
            return val;
        }

        float GetPickPointX() const { return mPick_Point[0]; }

        float GetPickPointY() const { return mPick_Point[1]; }

        bool GetDClickMessage()
        {
            auto val       = mDClick_Active;
            mDClick_Active = false;
            return val;
        }

        float GetDClickPointX() const { return mDClick_Point[0]; }

        float GetDClickPointY() const { return mDClick_Point[1]; }

        Vec3 GetPickPosWS() const { return mPick_PosWS; }

        void SetPickPosWS(const Vec3& pos)
        {
            // printf( "CH Picked WS %f %f %f\n", pos[0], pos[1], pos[2] );
            mPick_PosWS = pos;
        }

        bool OnMouseButton(Xfo_t& xf_CS_WS, int button, bool isDown, float x, float y)
        {
            if (button > (int)MAX_BUTTONS) return false;

            if (button == 0)
            {
                if (isDown)
                {
                    mLTouchTask.OnTouchBegin(x, y);
                    mCamRot_StaRotCS_WS = getRotSca(xf_CS_WS);
                    mCamRot_StaTraCS    = getTra(xf_CS_WS);
                    mCamRot_StaTraWS    = glm::inverse(mCamRot_StaRotCS_WS) * -mCamRot_StaTraCS;

                    if (const auto dTimeS = mButtPressTDeltas[(size_t)button].GetDeltaTimeS();
                        dTimeS && dTimeS < (1.0 / 3))
                    {
                        // printf( "Double click !\n" );
                        mDClick_Active = true;
                        mDClick_Point  = Vec2{x, y};

                        mPick_SkipOnce = true;
                    }
                }
                else
                {
                    mLTouchTask.OnTouchEnd(x, y);
                    // picking only if there is not double-click
                    if (!mPick_SkipOnce)
                    {
                        const auto off = mLTouchTask.GetTouchOff();
                        if (const auto lenSqr = glm::dot(off, off); lenSqr < (2.f * 2.f))
                        {
                            mPick_Active = true;
                            mPick_Point  = Vec2{x, y};
                            // switch to hook mode if something was picked
                            mMode        = MODE_HOOK;
                        }
                    }
                    mPick_SkipOnce = false;
                }
                return true;
            }
            else if (button == 1)
            {
                if (isDown)
                {
                    mRTouchTask.OnTouchBegin(x, y);
                    mCamPan_StaRot   = getRotSca(xf_CS_WS);
                    mCamPan_StaTraCS = getTra(xf_CS_WS);
                }
                else
                {
                    const auto off = mRTouchTask.GetTouchOff();
                    mRTouchTask.OnTouchEnd(x, y);
                    // return "false" if wasn't panning
                    if (const auto lenSqr = off[0] * off[0] + off[1] * off[1]; lenSqr < (2.f * 2.f)) return false;
                }
                return true;
            }
            return false;
        }

        static inline void rotateAround(Xfo_t& xf, float ang, const Vec3& axis, const Vec3& center)
        {
            xf = glm::translate(xf, center);
            xf = glm::rotate(xf, ang, axis);
            xf = glm::translate(xf, -center);
        }

        bool OnMouseMove(Xfo_t& xf_CS_WS, float x, float y, float dispW, float dispH)
        {
            if (!dispW) return false;

            if (mLTouchTask.IsTouchActive())
            {
                mLTouchTask.OnTouchMove((float)x, (float)y);

                const auto yaw        = 3.f * mLTouchTask.GetTouchOff()[0] / dispW;
                const auto ptc        = 3.f * mLTouchTask.GetTouchOff()[1] / dispH;

                const auto pivotPosWS = (mMode == MODE_HOOK ? mPick_PosWS : mCamRot_StaTraWS);

                Xfo_t xf{1.f};
                xf = glm::translate(xf, mCamRot_StaTraCS);
                rotateAround(xf, ptc, glm::vec3(1, 0, 0), mCamRot_StaRotCS_WS * pivotPosWS);
                xf *= Xfo_t(mCamRot_StaRotCS_WS);
                rotateAround(xf, yaw, glm::vec3(0, 1, 0), pivotPosWS);

                xf_CS_WS = xf;

                return true;
            }
            else if (mRTouchTask.IsTouchActive())
            {
                mRTouchTask.OnTouchMove((float)x, (float)y);

                const auto len  = glm::length(mCamPan_StaTraCS - mPick_PosWS);
                const auto offX = len * mRTouchTask.GetTouchOff()[0] / dispW;
                const auto offY = -len * mRTouchTask.GetTouchOff()[1] / dispH;

                auto tra        = mCamPan_StaTraCS;
                tra[0] += offX;
                tra[1] += offY;

                Xfo_t xf{1.f};
                xf = glm::translate(xf, tra); // camera pos in WS
                xf *= Xfo_t(mCamPan_StaRot);  // camera original rotation

                xf_CS_WS = xf;

                return true;
            }
            return false;
        }

        template <typename T> inline static T sign(T x) { return x < 0 ? (T)-1 : (T)1; }

        bool OnScroll(Xfo_t& xf_CS_WS, float x, float y)
        {
            (void)x;
            const auto rot_CS_WS = getRotSca(xf_CS_WS);
            const auto pos_CS    = getTra(xf_CS_WS);

            const auto camPosWS  = glm::inverse(rot_CS_WS) * -pos_CS;

            const auto distCS    = rot_CS_WS * (mPick_PosWS - camPosWS);

            const auto sca       = (double)-y;

            glm::dvec3 diff{};
            diff[0] = sca * sign(distCS[0]) * std::max(std::abs(distCS[0]), 0.1f);
            diff[1] = sca * sign(distCS[1]) * std::max(std::abs(distCS[1]), 0.1f);
            diff[2] = sca * distCS[2];

            mAccCS  = 0.0001 * diff;

            return true;
        }

        void AnimCamHand(Xfo_t& xf_CS_WS)
        {
            const auto dTimeS = mAnimTDelta.GetDeltaTimeS();
            if (dTimeS <= 0.0) return;

            // --- update pos speed
            mSpeedCS += mAccCS / dTimeS;

            // attenuate speed only if now key is being held down
            if (!mKeyPressMap['W'].ks_isDown && !mKeyPressMap['S'].ks_isDown) mSpeedCS[2] *= pow(0.01, dTimeS);

            if (!mKeyPressMap['A'].ks_isDown && !mKeyPressMap['D'].ks_isDown) mSpeedCS[0] *= pow(0.01, dTimeS);

            mSpeedCS[1] *= pow(0.01, dTimeS);

            // --- update rot speed
            mRotXYSpeed += mRotXYAcc / dTimeS;

            // attenuate speed only if now key is being held down
            if (!mKeyPressMap[mMappings.km_arrowL].ks_isDown && !mKeyPressMap[mMappings.km_arrowR].ks_isDown)
                mRotXYSpeed[1] *= pow(0.01, dTimeS);

            if (!mKeyPressMap[mMappings.km_arrowU].ks_isDown && !mKeyPressMap[mMappings.km_arrowD].ks_isDown)
                mRotXYSpeed[0] *= pow(0.01, dTimeS);

            mRotXYSpeed[0]       = std::clamp(mRotXYSpeed[0], -mLimits.li_maxRadSec[0], mLimits.li_maxRadSec[0]);

            mRotXYSpeed[1]       = std::clamp(mRotXYSpeed[1], -mLimits.li_maxRadSec[1], mLimits.li_maxRadSec[1]);

            const auto rot_CS_WS = getRotSca(xf_CS_WS);
            const auto rot_WS_CS = glm::inverse(rot_CS_WS);

            if (mMode == MODE_HOOK)
            {
                const auto camPosWS = rot_WS_CS * -getTra(xf_CS_WS);

                const auto distCS   = rot_CS_WS * (mPick_PosWS - camPosWS);
                if (distCS[2] > -0.25 && mSpeedCS[2] > 0) mSpeedCS *= -pow(0.00001, dTimeS);

                // printf( "%f\n", distCS[2] );
            }

            for (auto& [k, v] : mKeyPressMap)
                if (!v.ks_isDown) v.ks_strength *= pow(0.1, dTimeS);

            xf_CS_WS = glm::translate(
                xf_CS_WS, rot_WS_CS * glm::vec3((float)mSpeedCS[0], (float)mSpeedCS[1], (float)mSpeedCS[2]));

            const auto pivotPosWS =
                (mMode == MODE_HOOK ? mPick_PosWS : glm::inverse(getRotSca(xf_CS_WS)) * -getTra(xf_CS_WS));

            if (std::abs(mRotXYSpeed[1]) > 1e-6)
            {
                const auto ax = Vec3(0, 1, 0);
                rotateAround(xf_CS_WS, (float)mRotXYSpeed[1], ax, pivotPosWS);
            }
            if (std::abs(mRotXYSpeed[0]) > 1e-6)
            {
                const auto ax = glm::inverse(getRotSca(xf_CS_WS)) * Vec3(1, 0, 0);
                rotateAround(xf_CS_WS, (float)mRotXYSpeed[0], ax, pivotPosWS);
            }

            // roll speed is relative to yaw speed
            mRotXYSpeed[2] = mRotXYSpeed[1] / 2.0;
            // apply roll
            if (std::abs(mRotXYSpeed[2]) > 1e-6)
            {
                const auto ax = glm::inverse(getRotSca(xf_CS_WS)) * Vec3(0, 0, 1);
                rotateAround(xf_CS_WS, (float)mRotXYSpeed[2], ax, pivotPosWS);
            }
            // slowly strighten from roll
            if (const auto dirUp_WS_CS = getRotSca(xf_CS_WS) * Vec3(0, 1, 0); std::abs(dirUp_WS_CS[1]) > (PI_VAL / 50))
            {
                const auto angZ = PI_VAL / 2 - atan2(dirUp_WS_CS[1], dirUp_WS_CS[0]);

                const auto ax   = glm::inverse(getRotSca(xf_CS_WS)) * Vec3(0, 0, 1);
                rotateAround(xf_CS_WS, (float)angZ / 20, ax, pivotPosWS);
            }

            // reset the accelerations
            mAccCS    = {};
            mRotXYAcc = {};
        }

        bool OnKeyPress(int key, bool isDown)
        {
            auto& ks      = mKeyPressMap[key];
            ks.ks_isDown  = isDown;

            ks.ks_isAngle = key == mMappings.km_arrowL || key == mMappings.km_arrowR || key == mMappings.km_arrowU ||
                            key == mMappings.km_arrowD;

            auto sign = [](auto x) { return copysign(1.0, x); };

            if (isDown)
            {

                if (ks.ks_isAngle)
                {
                    ks.ks_strength = PI_VAL / 25000.0;

                    auto doMove    = [&](auto idx, double dir) {
                        mMode                 = MODE_FLY;

                        const bool isFlipping = sign(mRotXYSpeed[idx]) != sign(dir);
                        mRotXYSpeed[idx] *= (isFlipping ? 0.01 : 1.0);

                        const auto speedLen = glm::length(mSpeedCS);
                        const auto accLen   = pow(1 + 0.00005 * speedLen / 1, 0.7) - 1;
                        mRotXYAcc[idx]      = dir * std::max(ks.ks_strength, accLen);
                    };

                    if (mMappings.km_arrowU == key) doMove(0, 1.0);
                    if (mMappings.km_arrowD == key) doMove(0, -1.0);
                    if (mMappings.km_arrowR == key) doMove(1, 1.0);
                    if (mMappings.km_arrowL == key) doMove(1, -1.0);
                }
                else
                {
                    ks.ks_strength = 0.01;

                    auto doMove    = [&](auto idx, double dir) {
                        mMode                 = MODE_FLY;

                        const bool isFlipping = sign(mSpeedCS[idx]) != sign(dir);
                        mSpeedCS[idx] *= (isFlipping ? 0.01 : 1.0);

                        const auto speedLen = glm::length(mSpeedCS);
                        const auto accLen   = pow(1 + 0.03 * speedLen / 1, 0.9) - 1;
                        mAccCS[idx]         = dir * std::max(ks.ks_strength, accLen);
                    };

                    if ('W' == key) doMove(2, 1.0);
                    if ('S' == key) doMove(2, -1.0);
                    if ('A' == key) doMove(0, 1.0);
                    if ('D' == key) doMove(0, -1.0);
                }
            }

            return true;
        }
    };

} // namespace CH

#endif
