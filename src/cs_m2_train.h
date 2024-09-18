#ifndef CS_M2_TRAIN_H
#define CS_M2_TRAIN_H

#include <filesystem>
#include <functional>
#include <memory>
#include <vector>
#include "hdf5/hdf5_ext.h"
#include "log/log.h"
#include "ann_mlp_ga_v1.h"
#include "cs_m2_brain.h"
#include "cs_trainbase.h"
#include "cs_math.h"

#define CS_M2_TRAIN_USE_HDF5_CONFIG

//==================================================================
//==================================================================
class CS_M2_Train : public CS_TrainBase
{
    static constexpr const char* const CONFIGFILE = "./build/pathfinder.hd5";
    static constexpr const char* const nname      = "pathfinder/v1";

    // best chromos list just for display
    std::mutex mBestChromosMutex;
    std::vector<CS_Chromo> mBestChromos;
    std::vector<CS_ChromoInfo> mBestCInfos;

    std::unique_ptr<nn::ANN_MLP_GA<CS_SCALAR>> mNN;

  public:
    CS_M2_Train(size_t insN, size_t outsN)
        : CS_TrainBase(insN, outsN)
    {
#ifdef CS_M2_TRAIN_USE_HDF5_CONFIG
        if (!std::filesystem::exists(CONFIGFILE))
            throw std::runtime_error(std::string("File: ").append(CONFIGFILE).append(" not found. Exiting..."));
        h5::H5ppReader h5(CONFIGFILE);

        std::vector<size_t> nnsize, nnsizeH;
        h5.read(std::string(nname).append("/nn_hidden_layers"), nnsizeH);
        nnsize.push_back(mInsN);
        for (size_t i = 0; i < nnsizeH.size(); ++i) nnsize.push_back(nnsizeH[i]);
        nnsize.push_back(mOutsN);
        int seed;
        h5.read(std::string(nname).append("/seed"), seed);
        h5.read(std::string(nname).append("/nPop"), nPop);
        h5.read(std::string(nname).append("/nTop"), nTop);
        h5.read(std::string(nname).append("/nTopReport"), nTopReport);
        h5.read(std::string(nname).append("/save_period"), nPeriod);
        h5.read(std::string(nname).append("/save_periodic"), bSavePeriodic);
        h5.read(std::string(nname).append("/save_overwrite"), bSaveOverwrite);
        h5.read(std::string(nname).append("/save_path"), sSavePath);
        h5.read(std::string(nname).append("/save_name"), sSaveName);
#else
        const int seed = 7422;
        nPop = 100;
        nTop = 10;
        nTopReport = 10;
        nPeriod = 100;
        bSavePeriodic = true;
        bSaveOverwrite = true;
        sSavePath   = "./build/";
        sSaveName   = "pathfinder_m2";
        std::vector<size_t> nnsize, nnsizeH;
        nnsizeH.push_back(32);
        nnsize.push_back(mInsN);
        for (size_t i = 0; i < nnsizeH.size(); ++i)
            nnsize.push_back(nnsizeH[i]);
        nnsize.push_back(mOutsN);
#endif
        // create save directory if not exists
        if (bSavePeriodic)
            if (!std::filesystem::exists(sSavePath)) std::filesystem::create_directory(sSavePath);

        mNN = std::make_unique<nn::ANN_MLP_GA<CS_SCALAR>>(nnsize, seed, nPop, nTop, nn::TANH);
        mNN->SetName("pathfinder");
        mNN->SetMixed(false);
        mNN->CreatePopulation();
    }

    ~CS_M2_Train() override = default;

    //==================================================================
    unique_ptr<CS_BrainBase> CreateBrain(const CS_Chromo& chromo) override
    {
        return std::make_unique<CS_M2_Brain>(chromo, mInsN, mOutsN);
    }

    //==================================================================
    // initial list of chromosomes
    vector<CS_Chromo> MakeStartChromos() override
    {
        std::vector<CS_Chromo> chromos(nPop);
        for (size_t i = 0; i < nPop; ++i) chromos[i].SetChromoData(CS_M2_ChromoData{mNN.get(), i});

        return chromos;
    }

    //==================================================================
    // when an epoch has ended
    vector<CS_Chromo> OnEpochEnd(size_t epochIdx, const CS_Chromo* pChromos, const CS_ChromoInfo* pInfos,
                                 size_t n) override
    {
        (void)epochIdx;
        // sort by the cost
        std::vector<std::pair<const CS_Chromo*, const CS_ChromoInfo*>> pSorted;
        for (size_t i = 0; i < n; ++i) pSorted.push_back({pChromos + i, pInfos + i});

        std::sort(pSorted.begin(), pSorted.end(),
                  [](const auto& a, const auto& b) { return a.second->ci_cost < b.second->ci_cost; });

        // update the list of best chromosomes (with a lock... we're in a different thread)
        updateBestChromosList(pSorted);

        std::vector<size_t> chromoIds;
        chromoIds.reserve(pSorted.size());
        for (const auto& [pChromo, pInfo] : pSorted)
        {
            const auto* pData = pChromo->GetChromoData<CS_M2_ChromoData>();
            chromoIds.push_back(pData->mChromoIdx);
        }

        mNN->UpdateWeightsAndBiases(chromoIds);

        mNN->CreatePopulation();

        std::vector<CS_Chromo> newChromos(mNN->GetPopSize());
        for (size_t i = 0; i < newChromos.size(); ++i) newChromos[i].SetChromoData(CS_M2_ChromoData{mNN.get(), i});

        // save if necessary
        if (bSavePeriodic)
            if (!(epochIdx % nPeriod))
            {
                const std::string fname =
                    sSavePath + sSaveName + (bSaveOverwrite ? "" : "_" + std::to_string(epochIdx)) + ".hd5";
                mNN->Serialize(fname);
            }

        return newChromos;
    }

    //==================================================================
    void LockViewBestChromos(
        const std::function<void(const std::vector<CS_Chromo>&, const std::vector<CS_ChromoInfo>&)>& func) override
    {
        std::lock_guard<std::mutex> lock(mBestChromosMutex);
        func(mBestChromos, mBestCInfos);
    }

  private:
    //==================================================================
    void updateBestChromosList(const std::vector<std::pair<const CS_Chromo*, const CS_ChromoInfo*>>& pSorted)
    {
        std::lock_guard<std::mutex> lock(mBestChromosMutex);

        const auto n = std::min(nTopReport, pSorted.size());
        mBestChromos.resize(n);
        mBestCInfos.resize(n);
        for (size_t i = 0; i < n; ++i)
        {
            mBestChromos[i].SetChromoData(CS_M2_ChromoData{mNN.get(), i});

            mBestCInfos[i] = *pSorted[i].second;
        }
    }

    // config Variables
    size_t nPop, nTop, nTopReport, nPeriod;
    bool bSavePeriodic, bSaveOverwrite;
    std::string sSaveName, sSavePath;
};

#endif
