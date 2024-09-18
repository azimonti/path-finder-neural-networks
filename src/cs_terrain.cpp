/************************/
/*   cs_terrain.cpp     */
/*    Version 1.0       */
/*     2023/02/09       */
/************************/

#include "cs_terrain.h"
#include "ge_mesh2.h"
#include "geomprocessing.h"
#include "plasma2.h"

void to_json(nlohmann::json& j, const CS_Terrain::Params& v)
{
    j = nlohmann::json{
        CS_SERIALIZE_VAL(tp_fieldSize),        CS_SERIALIZE_VAL(tp_useImage),   CS_SERIALIZE_VAL(tp_image_path),
        CS_SERIALIZE_VAL(tp_noise_barrierLev), CS_SERIALIZE_VAL(tp_noise_seed), CS_SERIALIZE_VAL(tp_noise_roughness),
    };
}

void from_json(const nlohmann::json& j, CS_Terrain::Params& v)
{
    CS_DESERIALIZE_VAL(tp_fieldSize);
    CS_DESERIALIZE_VAL(tp_useImage);
    CS_DESERIALIZE_VAL(tp_image_path);
    CS_DESERIALIZE_VAL(tp_noise_barrierLev);
    CS_DESERIALIZE_VAL(tp_noise_seed);
    CS_DESERIALIZE_VAL(tp_noise_roughness);
}

CS_Terrain::CS_Terrain(const Params& par) : mCellSize(par.tp_fieldSize / TEX_SIZ), mPar(par)
{
    if (mPar.tp_useImage) ctor_makeHeightsFromImage();
    else ctor_makeHeightsFromNoise();

    moMeshF->OnGeometryUpdate();

    moMeshF->GetMaterial().mSpecularCol  = {0.1f, 0.1f, 0.1f};
    moMeshF->GetMaterial().mShininessPow = 20;

    // do the wired grid also
    moMeshW                              = gegt::MakeGridMeshWired({mPar.tp_fieldSize, mPar.tp_fieldSize}, {25, 25});

    // create the info mesh
    moMeshI                              = std::make_unique<ge::Mesh>();
    moMeshI->InitializeGeometryAttributes(false, ge::VTX_FLG_POS | ge::VTX_FLG_COL);
}

void CS_Terrain::ctor_makeHeightsFromNoise()
{
    mHeights.resize((size_t)1 << (TEX_SIZ_L2 * 2));

    Plasma2::Params p2par;
    p2par.pDest     = mHeights.data(); // destination values
    p2par.sizL2     = TEX_SIZ_L2;
    p2par.baseSizL2 = 3;
    p2par.seed      = mPar.tp_noise_seed;
    p2par.rough     = mPar.tp_noise_roughness;

    // generate the map
    Plasma2 plasma(p2par);
    while (plasma.IterateRow()) {}
    float mi = FLT_MAX;
    float ma = -FLT_MAX;
    for (const auto& samp : mHeights)
    {
        mi = std::min(mi, samp);
        ma = std::max(ma, samp);
    }
    const auto oomami = 1.f / (ma > mi ? ma - mi : 1.f);
    for (auto& samp : mHeights) samp = (samp - mi) * oomami;

    // make the mesh
    moMeshF           = std::make_unique<ge::Mesh>();

    const auto texSiz = (size_t)1 << TEX_SIZ_L2;

    const float hsiz  = mPar.tp_fieldSize / 2;

    auto scaleHeight  = [&](float h) -> float { return h > mPar.tp_noise_barrierLev ? h * 3.0f : h * 0.0f; };

    gegp::GenSolidMeshUV(*moMeshF, texSiz, texSiz,
                         [&](float u, float v) -> glm::vec4 // color (sample from plasma)
    {
        const auto iuv  = texSample(u, v, texSiz);
        const auto samp = mHeights[iuv[0] + iuv[1] * texSiz];
        if (samp > mPar.tp_noise_barrierLev)
        {
            return {remapRange(samp, mPar.tp_noise_barrierLev, 1.f, 0.50f, 0.20f),
                    remapRange(samp, mPar.tp_noise_barrierLev, 1.f, 0.50f, 0.30f),
                    remapRange(samp, mPar.tp_noise_barrierLev, 1.f, 0.50f, 0.10f), 1};
        }
        else
        {
            return {remapRange(samp, 0.f, mPar.tp_noise_barrierLev, 0.55f, 0.45f),
                    remapRange(samp, 0.f, mPar.tp_noise_barrierLev, 0.55f, 0.45f),
                    remapRange(samp, 0.f, mPar.tp_noise_barrierLev, 0.55f, 0.45f), 1};
        }
    },
                         [&](float u, float v) {
        const auto iuv = texSample(u, v, texSiz);
        const auto h   = mHeights[iuv[0] + iuv[1] * texSiz];
        return glm::vec3(glm::mix(-hsiz, hsiz, u), scaleHeight(h), glm::mix(-hsiz, hsiz, v));
    }, false);

    // apply height scaling definitively
    for (auto& h : mHeights) h = scaleHeight(h);
}

void CS_Terrain::ctor_makeHeightsFromImage() {}

void CS_Terrain::AddMeshesToSceneTerr(ge::Scene& scene)
{
    // render the grid
    scene.AddMesh(*moMeshF);
    // wire is slightly raised
    moMeshW->GetTransform() = ge::Transform().Translate({0, 0.05f, 0});
    scene.AddMesh(*moMeshW);
    // info is also slightly raised
    moMeshI->GetTransform() = ge::Transform().Translate({0, 0.05f, 0});
    scene.AddMesh(*moMeshI);
}

void CS_Terrain::DI_DrawCellAtPos(const glm::vec3& pos, const glm::vec4& color, const glm::vec4& borderCol)
{
    const auto cell     = getCellFromPos(pos);
    const auto srcVerts = getCellQuadVerts(cell);
    if (srcVerts)
    {
        const auto& svs = *srcVerts;
        gegp::AddQuad(moMeshI->GetPosVec(), svs[0], svs[1], svs[2], svs[3]);
        gegp::AddQuad(moMeshI->GetColVec(), color);
    }

    if (borderCol[3] <= 0) return;

    for (int iy = -1; iy <= 1; iy += 1)
    {
        for (int ix = -1; ix <= 1; ix += 1)
        {
            if (ix == 0 && iy == 0) continue;

            if (const auto srcVerts = getCellQuadVerts(cell + glm::ivec2(ix, iy)))
            {
                const auto& svs = *srcVerts;
                gegp::AddQuad(moMeshI->GetPosVec(), svs[0], svs[1], svs[2], svs[3]);
                gegp::AddQuad(moMeshI->GetColVec(), borderCol);
            }
        }
    }
}

void CS_Terrain::DI_Flush()
{
    moMeshI->UpdateBuffers(true);
    moMeshI->SetBBox(moMeshF->GetBBox());
}

float CS_Terrain::GetHeightFromPos(const glm::vec3& pos) const
{
    const auto cell = getCellFromPos(pos);
    return mHeights[(size_t)cell[0] + (size_t)cell[1] * TEX_SIZ];
}

bool CS_Terrain::IsPosInside(const glm::vec3& pos) const
{
    const auto cell = getCellFromPos_NoClamp(pos);
    return cell[0] >= 0 && (size_t)cell[0] < TEX_SIZ && cell[1] >= 0 && (size_t)cell[1] < TEX_SIZ;
}

glm::vec2 CS_Terrain::getUVFromPos(const glm::vec3& pos) const
{
    const auto hsiz     = mPar.tp_fieldSize / 2;
    const auto h_ce_siz = mCellSize * 0.5f;
    const float u       = remapRange(pos[0] + hsiz - h_ce_siz, 0, mPar.tp_fieldSize, 0, 1);
    const float v       = remapRange(pos[2] + hsiz - h_ce_siz, 0, mPar.tp_fieldSize, 0, 1);
    return {u, v};
}

glm::ivec2 CS_Terrain::getCellFromPos_NoClamp(const glm::vec3& pos) const
{
    const auto uv = getUVFromPos(pos);
    return {(int)((float)TEX_SIZ * uv[0]), (int)((float)TEX_SIZ * uv[1])};
}

glm::ivec2 CS_Terrain::getCellFromPos(const glm::vec3& pos) const
{
    const auto cell = getCellFromPos_NoClamp(pos);
    return {std::clamp(cell[0], 0, (int)TEX_SIZ - 1), std::clamp(cell[1], 0, (int)TEX_SIZ - 1)};
}

std::optional<std::array<glm::vec3, 4>> CS_Terrain::getCellQuadVerts(const glm::ivec2& cell) const
{
    if (cell[0] < 0 || cell[0] >= (int)TEX_SIZ - 1 || cell[1] < 0 || cell[1] >= (int)TEX_SIZ - 1) return {};

    const auto c0        = cell;
    const auto c1        = c0 + glm::ivec2(1, 0);
    const auto c2        = c0 + glm::ivec2(0, 1);
    const auto c3        = c0 + glm::ivec2(1, 1);
    const auto y0        = mHeights[(size_t)c0[0] + (size_t)c0[1] * TEX_SIZ];
    const auto y1        = mHeights[(size_t)c1[0] + (size_t)c1[1] * TEX_SIZ];
    const auto y2        = mHeights[(size_t)c2[0] + (size_t)c2[1] * TEX_SIZ];
    const auto y3        = mHeights[(size_t)c3[0] + (size_t)c3[1] * TEX_SIZ];
    const float hsiz     = mPar.tp_fieldSize / 2;
    const float h_ce_siz = mCellSize * 0.5f;
    const auto c0f       = glm::vec2(c0);
    const auto c1f       = glm::vec2(c1);
    const auto c2f       = glm::vec2(c2);
    const auto c3f       = glm::vec2(c3);
    return {
        {glm::vec3{c0f[0] * mCellSize - hsiz + h_ce_siz, y0, c0f[1] * mCellSize - hsiz + h_ce_siz},
         glm::vec3{c1f[0] * mCellSize - hsiz + h_ce_siz, y1, c1f[1] * mCellSize - hsiz + h_ce_siz},
         glm::vec3{c2f[0] * mCellSize - hsiz + h_ce_siz, y2, c2f[1] * mCellSize - hsiz + h_ce_siz},
         glm::vec3{c3f[0] * mCellSize - hsiz + h_ce_siz, y3, c3f[1] * mCellSize - hsiz + h_ce_siz}}
    };
}
