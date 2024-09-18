#ifndef CS_BRAINBASE_H
#define CS_BRAINBASE_H

#include <vector>
#include "cs_chromo.h"
#include "cs_math.h"

class CS_BrainBase
{
  public:
    CS_BrainBase(const CS_Chromo& chromo, size_t insN, size_t outsN)
    {
        (void)chromo;
        (void)insN;
        (void)outsN;
    }

    CS_BrainBase(uint32_t seed, size_t insN, size_t outsN)
    {
        (void)seed;
        (void)insN;
        (void)outsN;
    }

    virtual ~CS_BrainBase() {}

    virtual CS_Chromo MakeBrainChromo() const { return {}; }

    virtual void AnimateBrain(const CSM_Vec& ins, CSM_Vec& outs) const = 0;
};

#endif
