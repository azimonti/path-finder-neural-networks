#ifndef CS_SCENARIOTERRSETUP_H
#define CS_SCENARIOTERRSETUP_H

#include <vector>
#include "cs_serialize_fwd.h"
#include "cs_terrain.h"

class CS_ScenarioTerrSetup
{
    CS_Terrain::Params mTPar;
    std::vector<uint32_t> mSeeds;

  public:
    CS_ScenarioTerrSetup() = default;

    CS_ScenarioTerrSetup(const CS_Terrain::Params& tPar, const std::vector<uint32_t>& seeds)
        : mTPar(tPar), mSeeds(seeds)
    {
        if (mSeeds.empty()) mSeeds.push_back(mTPar.tp_noise_seed);
    }

    std::vector<CS_Terrain::Params> MakeVariants() const;

    bool DrawTerrSetupUI(bool allowMultipleVariants);

    friend void to_json(nlohmann::json& j, const CS_ScenarioTerrSetup& v);
    friend void from_json(const nlohmann::json& j, CS_ScenarioTerrSetup& v);
};

#endif
