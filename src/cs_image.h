#ifndef CS_IMAGE_H
#define CS_IMAGE_H

#include <array>
#include <cassert>
#include <vector>
#include "cs_math.h"

using CS_RGB8  = std::array<uint8_t, 3>;
using CS_RGBA8 = std::array<uint8_t, 4>;

class CS_Image
{
    std::vector<uint8_t> mData;

    size_t mWD{};
    size_t mHE{};
    size_t mChansN{};
    size_t mBytesPC{};
    size_t mBytesPP{};

  public:
    CS_Image(size_t w, size_t h, size_t chansN, size_t bytesPC = 1)
        : mWD(w), mHE(h), mChansN(chansN), mBytesPC(bytesPC), mBytesPP(chansN * bytesPC)
    {
        mData.resize(w * h * chansN);
    }

    ~CS_Image();

    void SetSize(int width, int height);

    void SetPixelRGB8(int x, int y, const CS_RGB8& col)
    {
        assert(x >= 0 && x < mWD && y >= 0 && y < mHE);
        assert(mBytesPC == 1 && mChansN == 3);
        auto* p = &mData[(y * mWD + x) * mBytesPP];
        p[0]    = col[0];
        p[1]    = col[1];
        p[2]    = col[2];
    }

    void SetPixelRGBA8(int x, int y, const CS_RGBA8& col)
    {
        assert(x >= 0 && x < mWD && y >= 0 && y < mHE);
        assert(mBytesPC == 1 && mChansN == 4);
        auto* p = &mData[(y * mWD + x) * mBytesPP];
        p[0]    = col[0];
        p[1]    = col[1];
        p[2]    = col[2];
        p[3]    = col[3];
    }

    size_t GetWidth() const { return mWD; }

    size_t GetHeight() const { return mHE; }

    const uint8_t* GetPixelPtr(int x, int y) const
    {
        assert(x >= 0 && x < mWD && y >= 0 && y < mHE);
        return &mData[(y * mWD + x) * mBytesPP];
    }

    CS_RGB8 GetPixelRGB8(int x, int y) const
    {
        assert(mBytesPC == 1 && mChansN == 3);
        const auto* p = GetPixelPtr(x, y);
        return CS_RGB8{p[0], p[1], p[2]};
    }

    CS_RGBA8 GetPixelRGBA8(int x, int y) const
    {
        assert(mBytesPC == 1 && mChansN == 4);
        const auto* p = GetPixelPtr(x, y);
        return CS_RGBA8{p[0], p[1], p[2], p[3]};
    }
}

#endif
