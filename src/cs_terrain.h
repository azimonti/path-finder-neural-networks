#ifndef _CS_TERRAIN_H_31E35B485D21438B8680189EF7D185F3_
#define _CS_TERRAIN_H_31E35B485D21438B8680189EF7D185F3_

/************************/
/*     cs_terrain.h     */
/*    Version 1.0       */
/*     2022/07/07       */
/************************/

#include <algorithm>
#include <functional>
#include <optional>
#include <vector>
#include "cs_serialize.h"
#include "cs_serialize_fwd.h"
#include "mesh.h"
#include "scene.h"

class CS_Terrain
{
    static constexpr size_t TEX_SIZ_L2 = 9;
    static constexpr size_t TEX_SIZ    = (size_t)1 << TEX_SIZ_L2;

    const float mCellSize;

    std::vector<float> mHeights;

  public:
    std::unique_ptr<ge::Mesh> moMeshW;
    std::unique_ptr<ge::Mesh> moMeshF;
    std::unique_ptr<ge::Mesh> moMeshI;

  public:
    struct Params
    {
        float tp_fieldSize        = 100.f;
        bool tp_useImage          = false;
        std::string tp_image_path = "assets/terrain_track_01.png";
        float tp_noise_barrierLev = 0.5f;
        uint32_t tp_noise_seed    = 102;
        float tp_noise_roughness  = 0.50f;

        friend void to_json(nlohmann::json& j, const Params& v);
        friend void from_json(const nlohmann::json& j, Params& v);
    };

  private:
    const Params mPar;

  public:
    CS_Terrain(const Params& par);

  private:
    void ctor_makeHeightsFromNoise();
    void ctor_makeHeightsFromImage();

  public:
    void AddMeshesToSceneTerr(ge::Scene& scene);

    void DI_DrawCellAtPos(const glm::vec3& pos, const glm::vec4& color, const glm::vec4& borderCol = {0, 0, 0, 0});
    void DI_Flush();

    float GetCellSize() const { return mCellSize; }

    float GetFieldSize() const { return mPar.tp_fieldSize; }

    float GetHeightFromPos(const glm::vec3& pos) const;
    bool IsPosInside(const glm::vec3& pos) const;

    void ScanRay(const glm::vec3& staPos, const glm::vec3& endPos,
                 const std::function<bool(const glm::vec3&, float, bool)>& callback) const;

    glm::vec2 getUVFromPos(const glm::vec3& pos) const;
    glm::ivec2 getCellFromPos(const glm::vec3& pos) const;
    glm::ivec2 getCellFromPos_NoClamp(const glm::vec3& pos) const;
    glm::vec3 calcPosFromCell(const glm::ivec2& cell) const;

    std::optional<std::array<glm::vec3, 4>> getCellQuadVerts(const glm::ivec2& cell) const;

  private:
    static std::array<size_t, 2> texSample(float u, float v, size_t texSiz)
    {
        return {(size_t)std::clamp((float)texSiz * u, 0.f, (float)(texSiz - 1)),
                (size_t)std::clamp((float)texSiz * v, 0.f, (float)(texSiz - 1))};
    }

    static float remapRange(float v, float srcL, float srcR, float desL, float desR)
    {
        const auto t = (v - srcL) / (srcR - srcL);
        return glm::mix(desL, desR, t);
    }
};

inline glm::vec3 CS_Terrain::calcPosFromCell(const glm::ivec2& cell) const
{
    const auto hsiz     = mPar.tp_fieldSize / 2;
    const auto h_ce_siz = mCellSize * 0.5f;
    const float x       = remapRange((float)cell[0], 0, (float)TEX_SIZ, -hsiz + h_ce_siz, hsiz - h_ce_siz);
    const float z       = remapRange((float)cell[1], 0, (float)TEX_SIZ, -hsiz + h_ce_siz, hsiz - h_ce_siz);
    return {x, 0, z};
}

inline void CS_Terrain::ScanRay(const glm::vec3& staPos, const glm::vec3& endPos,
                                const std::function<bool(const glm::vec3&, float, bool)>& callback) const
{
    auto isCellOutsideMap = [](const glm::ivec2& cell) {
        return cell[0] < 0 || cell[0] >= (int)TEX_SIZ || cell[1] < 0 || cell[1] >= (int)TEX_SIZ;
    };

    auto staCell = getCellFromPos_NoClamp(staPos);
    auto endCell = getCellFromPos_NoClamp(endPos);
    if (staCell == endCell)
    {
        callback(staPos, GetHeightFromPos(staPos), isCellOutsideMap(staCell));
        return;
    }

    //  for now ensure that 0 is the major axis
    bool swapped = false;
    if (std::abs(endCell[0] - staCell[0]) < std::abs(endCell[1] - staCell[1]))
    {
        swapped = true;
        std::swap(staCell[0], staCell[1]);
        std::swap(endCell[0], endCell[1]);
    }

    const auto step0 = endCell[0] > staCell[0] ? 1 : -1;
    const auto dir   = endPos - staPos;
    const auto step1 = dir[2] / dir[0];
    (void)step1;

    const auto ooLen = 1.f / (float)(endCell[0] - staCell[0]);

    // along the major axis...
    for (auto i0 = staCell[0]; i0 != (endCell[0] + step0); i0 += step0)
    {
        // 0..1
        const auto t = (float)(i0 - staCell[0]) * ooLen;

        // cell index of the minor axis (round at center of the cell)
        const auto i2 =
            std::clamp((int)std::floor(glm::mix((float)staCell[1], (float)endCell[1], t) + 0.5f), 0, (int)TEX_SIZ - 1);

        const auto useI0 = !swapped ? i0 : i2;
        const auto useI2 = !swapped ? i2 : i0;

        const auto cell  = glm::ivec2{useI0, useI2};
        const auto pos   = calcPosFromCell(cell);

        if (isCellOutsideMap(cell))
        {
            if (!callback(pos, 0, true)) break;
        }
        else
        {
            const auto h = mHeights[(size_t)cell[0] + (size_t)cell[1] * TEX_SIZ];
            if (!callback(pos, h, false)) break;
        }
    }
}

#endif
