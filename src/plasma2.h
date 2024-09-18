#ifndef _PLASMA2_H_5623F0BA830442B6AE63288B9892F0E9_
#define _PLASMA2_H_5623F0BA830442B6AE63288B9892F0E9_

/************************/
/*     plasma2.h        */
/*    Version 1.0       */
/*     2022/06/25       */
/************************/

#include <memory>
#include <vector>
#include <stdint.h>

class Rand2D;

class Plasma2
{
    std::vector<float> mBaseGrid;
    const size_t BLOCKS_NL2{0};

    std::unique_ptr<Rand2D> mGen_oRandPool;
    size_t mGen_IterIX{0};
    size_t mGen_IterIY{0};

  public:
    struct Params
    {
        float* pDest{};
        size_t sizL2{8};
        size_t baseSizL2{4};
        uint32_t seed{};
        float sca{1};
        float rough{0.5f};
    };

  private:
    Params mPar;

  public:
    Plasma2(Params& par);
    ~Plasma2();

    void RendBlock(size_t ix, size_t iy);

    bool IterateBlock();
    bool IterateRow();
};

#endif
