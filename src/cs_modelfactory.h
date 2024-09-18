#ifndef CS_MODELFACTORY_H
#define CS_MODELFACTORY_H

#include <memory>
#include <string>
#include <vector>
#include "cs_brainbase.h"
#include "cs_trainbase.h"

namespace CS_ModelFactory
{

    size_t GetModelsN();

    std::string GetModelName(size_t idx);

    std::unique_ptr<CS_BrainBase> CreateBrain(size_t modelIdx, const CS_Chromo& chromo, size_t insN, size_t outsN);

    std::unique_ptr<CS_TrainBase> CreateTrain(size_t modelIdx, size_t insN, size_t outsN);

} // namespace CS_ModelFactory

#endif
