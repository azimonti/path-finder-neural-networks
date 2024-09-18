#ifndef _CS_UTILS_H_DEEBC600BD95430186C5C88584DAD9C2_
#define _CS_UTILS_H_DEEBC600BD95430186C5C88584DAD9C2_

/************************/
/*      cs_utils.h      */
/*    Version 1.0       */
/*     2022/06/26       */
/************************/

#include <array>
#include <cmath>
#include <string>
#include "ge_mathbase.h"
#include "uibase.h"

inline std::string CS_MakeRaceName(size_t tidx)
{
    return std::string("Race Group ") + (char)('A' + tidx);
}

inline ge::Color CS_MakeRaceCol(size_t tidx)
{
    static const std::array<ge::Color, 3> sRaceCols{ge::Color(1.0f, 0.2f, 0.2f, 1), ge::Color(0.2f, 0.2f, 1.0f, 1),
                                                    ge::Color(0.2f, 1.0f, 0.2f, 1)};
    return sRaceCols[tidx % std::size(sRaceCols)];
}

inline ImVec4 CS_MakeRaceColGUI(size_t tidx)
{
    const auto c = CS_MakeRaceCol(tidx);
    return ImVec4(c[0], c[1], c[2], c[3]);
}

inline auto CS_MakeCostString = [](double cost) {
    // display the cost by orders of magnitude
    const auto mag    = (int)std::floor(std::log10(cost));

    const auto magStr = mag >= 0 ? "10^" + std::to_string(mag) : "10^-" + std::to_string(-mag);

    const auto costStr =
        mag >= 0 ? std::to_string(cost / std::pow(10, mag)) : std::to_string(cost * std::pow(10, -mag));

    return costStr + " " + magStr;
};

#endif
