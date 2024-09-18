#include "cs_m1_train.h"
#include "cs_m2_train.h"
#include "cs_modelfactory.h"

namespace CS_ModelFactory
{

    static std::vector<std::string> _sModelNames = {"Model 1", "Model 2"};

    size_t GetModelsN()
    {
        return 2;
    }

    std::string GetModelName(size_t idx)
    {
        return _sModelNames[idx];
    }

    std::unique_ptr<CS_BrainBase> CreateBrain(size_t modelIdx, const CS_Chromo& chromo, size_t insN, size_t outsN)
    {
        switch (modelIdx)
        {
        case 0: return std::make_unique<CS_M1_Brain>(chromo, insN, outsN);
        case 1: return std::make_unique<CS_M2_Brain>(chromo, insN, outsN);
        default: throw std::runtime_error("Unknown model index");
        }
    }

    std::unique_ptr<CS_TrainBase> CreateTrain(size_t modelIdx, size_t insN, size_t outsN)
    {
        switch (modelIdx)
        {
        case 0: return std::make_unique<CS_M1_Train>(insN, outsN);
        case 1: return std::make_unique<CS_M2_Train>(insN, outsN);
        default: throw std::runtime_error("Unknown model index");
        }
    }

} // namespace CS_ModelFactory
