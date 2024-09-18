#ifndef _UIBASE_H_70397A50BBAB4C7B85F5DC79F0C646B4_
#define _UIBASE_H_70397A50BBAB4C7B85F5DC79F0C646B4_

/************************/
/*       uibase.h       */
/*    Version 1.0       */
/*     2022/07/07       */
/************************/

#include <string>
#include <vector>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"

static const auto UIB_COL_RED   = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
static const auto UIB_COL_GREEN = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
static const auto UIB_COL_WHITE = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
// static const auto UIB_COL_ORANGE = ImVec4(1.0f, 0.5f, 0.0f, 1.0f);

inline float UIB_ContentSca     = 1.0f;

inline bool UIB_Header(const std::string& name, bool defOpen = true, bool addGap = false)
{
    if (addGap)
    {
        ImGui::Separator();
        ImGui::NewLine();
    }

    return ImGui::CollapsingHeader((name + "##head").c_str(), defOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0);
}

class UIB_EasyTable
{
  public:
    UIB_EasyTable(const std::vector<const char*> pTitles) : mColsN(pTitles.size())
    {
        for (const char* p : pTitles) ImGui::TableSetupColumn(p);
        ImGui::TableHeadersRow();
        ImGui::TableNextRow();
        mCurColsN = pTitles.size();
    }

    void NextCol()
    {
        if (mCurColsN++ >= mColsN)
        {
            mCurColsN = 1;
            ImGui::TableNextRow();
        }
        ImGui::TableNextColumn();
    }

    void AddText(const std::string& txt)
    {
        NextCol();
        if (mNextColor.x == mNullColor.x && mNextColor.y == mNullColor.y && mNextColor.z == mNullColor.z &&
            mNextColor.w == mNullColor.w)
            ImGui::Text("%s", txt.c_str());
        else ImGui::TextColored(mNextColor, "%s", txt.c_str());
        mNextColor = mNullColor;
    }

    void SetNextColor(const ImVec4& col) { mNextColor = col; }

  private:
    const size_t mColsN;
    const ImVec4 mNullColor{};

    size_t mCurColsN{};

    ImVec4 mNextColor{};
};

inline bool UIB_InputSizeT(const std::string& s, size_t* pVal, size_t* pStep = 0, size_t* pStepFast = 0)
{
    return ImGui::InputScalar(s.c_str(), ImGuiDataType_U64, pVal, pStep, pStepFast, "%zu");
}

inline bool UIB_InputUInt(const std::string& s, unsigned int* pVal, unsigned int* pStep = 0,
                          unsigned int* pStepFast = 0)
{
    return ImGui::InputScalar(s.c_str(), ImGuiDataType_U32, pVal, pStep, pStepFast, "%u");
}

inline bool UIB_SlideDouble(const std::string& s, double* pVal, double min, double max, const char* pFmt = 0)
{
    return ImGui::SliderScalar(s.c_str(), ImGuiDataType_Double, pVal, &min, &max, pFmt);
}

inline bool UIB_InputPath(const std::string& s, std::string& io_val)
{
    return ImGui::InputText(s.c_str(), &io_val, ImGuiInputTextFlags_EnterReturnsTrue);
}

inline void UIB_PushDisabled()
{
    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
}

inline void UIB_PopDisabled()
{
    ImGui::PopItemFlag();
    ImGui::PopStyleVar();
}

#endif
