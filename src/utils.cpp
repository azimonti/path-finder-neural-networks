/************************/
/*      utils.cpp       */
/*    Version 1.0       */
/*     2022/06/26       */
/************************/

#include <chrono>
#include "utils.h"

namespace ut
{

    double GetEpochTimeS()
    {
        return (double)std::chrono::duration_cast<std::chrono::microseconds>(
                   std::chrono::system_clock::now().time_since_epoch())
                   .count() /
               1e6;
    }

    double GetSteadyTimeS()
    {
        return (double)std::chrono::duration_cast<std::chrono::microseconds>(
                   std::chrono::steady_clock::now().time_since_epoch())
                   .count() /
               1e6;
    }

} // namespace ut
