#ifndef CS_PLAYER
#define CS_PLAYER

#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <vector>
#include "cs_brainbase.h"
#include "cs_sim.h"

class CS_Player
{
    std::unique_ptr<CS_BrainBase> moBrain;
    std::unique_ptr<CS_Sim> moSim;
    std::string mChromoHex;

  public:
    using CreateSimFnT = std::function<std::unique_ptr<CS_Sim>(const CS_BrainBase&)>;

    struct Params
    {
        double chromoCost{};
        std::string chromoHex;
        CreateSimFnT createSimFn;
    };

  private:
    Params mPar;

  public:
    CS_Player(const Params& par, std::unique_ptr<CS_BrainBase>&& oBrain) : moBrain(std::move(oBrain)), mPar(par)
    {
        moSim = par.createSimFn(*moBrain);
    }

    void AnimPlayer(double speedFactor, bool doDraw)
    {
        if (moSim->IsSimComplete()) return;

        const auto loopN = (size_t)speedFactor;

        for (size_t i = 0; i < loopN && !moSim->IsSimComplete(); ++i) moSim->AnimSim(1.0 / 60, doDraw && i == 0);
    }

    void AddMeshesToScenePlayer(ge::Scene& scene) const { moSim->AddMeshesToSceneSim(scene); }

    auto& GetPlayerSim() { return *moSim; }

    const auto& GetPlayerBrain() const { return *moBrain; }

    const std::string& GetChromoHex() const { return mPar.chromoHex; }

    double GetChromoCost() const { return mPar.chromoCost; }
};

#endif
