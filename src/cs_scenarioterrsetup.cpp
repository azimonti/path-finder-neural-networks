#include "cs_scenarioterrsetup.h"
#include "cs_serialize.h"
#include "uibase.h"

void to_json(nlohmann::json& j, const CS_ScenarioTerrSetup& v)
{
    j = nlohmann::json{
        CS_SERIALIZE_VAL(mTPar),
        CS_SERIALIZE_VAL(mSeeds),
    };
}

void from_json(const nlohmann::json& j, CS_ScenarioTerrSetup& v)
{
    CS_DESERIALIZE_VAL(mTPar);
    CS_DESERIALIZE_VAL(mSeeds);
}

static bool drawWrappedIntegerInputFields(const char* pLabel, std::vector<uint32_t>& values, float indentW = 20,
                                          float inputW = 50)
{
    bool rebuild{};

    // const auto id = pLabel + std::to_string((ptrdiff_t)&values);
    // ImGui::PushID(id.c_str());

    const auto useInputW  = inputW * UIB_ContentSca;
    const auto useIndentW = indentW * UIB_ContentSca;

    // Add a field for the number of items
    auto curSize          = (int)values.size();
    ImGui::Text("%s", pLabel);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(useInputW * 2);
    if (ImGui::InputInt("##curSize", &curSize, 1, 10))
    {
        curSize             = std::max(1, std::min(10, curSize));
        const auto oldSize  = (int)values.size();
        rebuild             = true;

        // add a sequence of numbers, so that we don't have just 0s
        const auto startVal = (uint32_t)(values.size() ? values.back() : 0);

        values.resize((size_t)curSize);
        for (int i = oldSize; i < curSize; ++i) values[(size_t)i] = startVal + (uint32_t)(i - oldSize + 1);
    }

    // Add some indentation
    ImGui::Dummy(ImVec2(useIndentW, 0.0f));
    ImGui::SameLine();

    const auto windowW              = ImGui::GetContentRegionAvail().x;
    const auto inputFieldSpacing    = ImGui::GetStyle().ItemSpacing.x;
    const auto inputFieldTotalWidth = useInputW + inputFieldSpacing;

    float currentXPos               = useIndentW;
    for (size_t i = 0; i < values.size(); ++i)
    {
        ImGui::PushID(static_cast<int>(i));
        ImGui::SetNextItemWidth(useInputW);
        if (ImGui::InputScalar("", ImGuiDataType_U32, &values[i])) rebuild = true;

        // Check if the next widget would go outside the window area
        currentXPos += inputFieldTotalWidth;
        if (currentXPos + inputFieldTotalWidth >= windowW)
        {
            currentXPos = useIndentW;
            ImGui::Dummy(ImVec2(useIndentW, 0.0f));
        }
        if ((i + 1) < values.size()) ImGui::SameLine();

        ImGui::PopID();
    }

    // ImGui::PopID();

    return rebuild;
}

bool CS_ScenarioTerrSetup::DrawTerrSetupUI(bool allowMultipleVariants)
{
    if (!UIB_Header("Terrain Setup", true)) return false;

    bool rebuild{};
    rebuild |= ImGui::InputFloat("Field Size", &mTPar.tp_fieldSize, 1.f, 10.0f);
    mTPar.tp_fieldSize = std::clamp(mTPar.tp_fieldSize, 10.f, 1000.f);

    if (ImGui::RadioButton("Use Image", mTPar.tp_useImage == true))
    {
        mTPar.tp_useImage = true;
        rebuild           = true;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Use Noise", mTPar.tp_useImage == false))
    {
        mTPar.tp_useImage = false;
        rebuild           = true;
    }

    if (mTPar.tp_useImage) { rebuild |= UIB_InputPath("Image Path", mTPar.tp_image_path); }
    else
    {
        rebuild |= ImGui::InputFloat("Barrier Level", &mTPar.tp_noise_barrierLev, 0.0f, 1.0f);
        if (allowMultipleVariants) { rebuild |= drawWrappedIntegerInputFields("Seeds:", mSeeds); }
        else
        {
            mSeeds.resize(1);
            rebuild |= ImGui::InputScalar("Seed", ImGuiDataType_U32, &mSeeds[0]);
        }

        rebuild |= ImGui::InputFloat("Roughness", &mTPar.tp_noise_roughness, 0.0f, 1.0f);
    }

    return rebuild;
}

std::vector<CS_Terrain::Params> CS_ScenarioTerrSetup::MakeVariants() const
{
    std::vector<CS_Terrain::Params> vars;

    if (mTPar.tp_useImage) { vars.push_back(mTPar); }
    else
    {
        for (const auto seed : mSeeds)
        {
            auto var          = mTPar;
            var.tp_noise_seed = seed;
            vars.push_back(var);
        }
    }

    return vars;
}
