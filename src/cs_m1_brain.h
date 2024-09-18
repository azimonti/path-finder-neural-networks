#ifndef CS_M1_BRAIN_H
#define CS_M1_BRAIN_H

#include <memory>
#include "cs_brainbase.h"
#include "cs_math.h"

template <typename T> class SimpleNN;

class CS_M1_Brain : public CS_BrainBase
{
    std::unique_ptr<SimpleNN<CS_SCALAR>> moNN;

  public:
    CS_M1_Brain(const CS_Chromo& chromo, size_t insN, size_t outsN);
    CS_M1_Brain(uint32_t seed, size_t insN, size_t outsN);
    ~CS_M1_Brain();

    CS_Chromo MakeBrainChromo() const override;

    void AnimateBrain(const CSM_Vec& ins, CSM_Vec& outs) const override;
};

#endif
