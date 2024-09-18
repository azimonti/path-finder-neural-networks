#ifndef CS_TYPES_H
#define CS_TYPES_H

#include "cs_math.h"

static constexpr size_t CS_SENS_PROBES_N = 7;

// tags only really used for hand-made brains
enum CS_SensorType : int {
    CS_SENS_POS_X,
    CS_SENS_POS_Z,
    CS_SENS_VEL_X,
    CS_SENS_VEL_Z,
    CS_SENS_FWD_X,
    CS_SENS_FWD_Z,
    CS_SENS_TARGET_X,
    CS_SENS_TARGET_Z,
    CS_SENS_PROBE_UNIT,
    CS_SENS_PROBE_FIRST_HITDIST,
    CS_SENS_PROBE_LAST_HITDIST = CS_SENS_PROBE_FIRST_HITDIST + (int)CS_SENS_PROBES_N - 1,
    CS_SENS_IS_OUTSIDE_MAP,
    CS_SENS_IS_IN_DEAD_ZONE,
    CS_SENS_N
};

enum CS_ControlType : int {
    CS_CTRL_FACCEL,
    CS_CTRL_BACCEL,
    CS_CTRL_BRAKE,
    CS_CTRL_STEER_L,
    CS_CTRL_STEER_R,
    CS_CTRL_N
};

[[maybe_unused]] static inline float CS_CTRL_MIN_VAL = 0.0f;
[[maybe_unused]] static inline float CS_CTRL_MAX_VAL = 1.0f;

inline const char* CS_GetControlName(CS_ControlType type)
{
    switch (type)
    {
    case CS_CTRL_FACCEL: return "F Accel";
    case CS_CTRL_BACCEL: return "B Accel";
    case CS_CTRL_BRAKE: return "Brake";
    case CS_CTRL_STEER_L: return "Steer L";
    case CS_CTRL_STEER_R: return "Steer R";
    default: return "UNKNOWN";
    }
}

#endif
