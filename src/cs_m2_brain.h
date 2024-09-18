#ifndef CS_M2_BRAIN_H
#define CS_M2_BRAIN_H

#include <memory>
#include "cs_brainbase.h"
#include "cs_math.h"

namespace nn
{
    template <typename T> class ANN_MLP_GA;
}

struct CS_M2_ChromoData
{
    nn::ANN_MLP_GA<CS_SCALAR>* mpNN{};
    size_t mChromoIdx{};
};

class CS_M2_Brain : public CS_BrainBase
{
    const CS_M2_ChromoData mChromoData;

  public:
    CS_M2_Brain(const CS_Chromo& chromo, size_t insN, size_t outsN);
    CS_M2_Brain(uint32_t seed, size_t insN, size_t outsN);
    ~CS_M2_Brain();

    CS_Chromo MakeBrainChromo() const override;

    void AnimateBrain(const CSM_Vec& ins, CSM_Vec& outs) const override;
};

#endif
