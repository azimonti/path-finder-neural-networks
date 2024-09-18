#include <cassert>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>
#include "ann_mlp_ga_v1.h"
#include "cs_m2_brain.h"
#include "cs_math.h"

CS_M2_Brain::CS_M2_Brain(const CS_Chromo& chromo, size_t insN, size_t outsN)
    : CS_BrainBase(chromo, insN, outsN), mChromoData(*chromo.GetChromoData<CS_M2_ChromoData>())
{
}

CS_M2_Brain::CS_M2_Brain(uint32_t seed, size_t insN, size_t outsN) : CS_BrainBase(seed, insN, outsN)
{
    assert(false);
}

CS_M2_Brain::~CS_M2_Brain() = default;

CS_Chromo CS_M2_Brain::MakeBrainChromo() const
{
    assert(false);
    return {};
}

void CS_M2_Brain::AnimateBrain(const CSM_Vec& ins, CSM_Vec& outs) const
{
    mChromoData.mpNN->feedforward(ins.data(), ins.size(), outs.data(), outs.size(), mChromoData.mChromoIdx, false);
}
