/**************************/
/*  imguizmo_dialog.cpp   */
/*      Version 1.0       */
/*       2022/06/24       */
/**************************/

#include "log/log.h"
#include "gl_app.h"
#include "imgui.h"
#include "imguizmo_dialog.h"
#include "imGuIZMOquat.h"
#include "mesh.h"
#include "scene.h"

ImguizmoDialog::ImguizmoDialog() : d_visible(false), h_visible(false) {}

ImguizmoDialog::~ImguizmoDialog() {}

#define USE_LIGHT_POV_FOR_LIGHT

static void setTextCol(float r, float g, float b)
{
    ImGuiStyle& style             = ImGui::GetStyle();
    style.Colors[ImGuiCol_Text].x = r;
    style.Colors[ImGuiCol_Text].y = g;
    style.Colors[ImGuiCol_Text].z = b;
}

static void setTextCol(const ImVec4& col)
{
    ImGuiStyle& style           = ImGui::GetStyle();
    style.Colors[ImGuiCol_Text] = col;
}

void ImguizmoDialog::onRender(GLFWwindow* window)
{
    GLApp* w = (GLApp*)glfwGetWindowUserPointer(window);
    if (!d_visible) { return; }

    int width, height;
    glfwGetWindowSize(window, &width, &height);
    ImGuiStyle& style       = ImGui::GetStyle();

    auto& scene             = w->GetEngineWnd()->GetScene();

    // Camera
    ge::Transform& xformCam = w->GetEngineWnd()->GetCamera().GetTransform();
    glm::quat qtin2         = xformCam.GetMatrix(); // NOTE: implicit mat4 -> quat conversion (rotation only);
    glm::vec3 posin2        = xformCam.GetTranslation();

    // Object
    auto* pObject           = w->GetEngineWnd()->GetObjMesh();
    ge::Transform dummyObjXForm;
    ge::Transform& xformObj = pObject ? pObject->GetTransform() : dummyObjXForm;
    glm::quat objQua        = xformObj.GetMatrix();
    objQua                  = glm::normalize(qtin2 * objQua);
    glm::vec3 objPos        = xformObj.GetTranslation();

    // Light
#ifdef USE_LIGHT_POV_FOR_LIGHT
    glm::vec3 lightDirWS = glm::normalize(qtin2 * scene.GetDirLight().GetDirWS());
#else
    glm::vec3 lightDirWS = glm::normalize(scene.GetDirLight().GetDirWS());
#endif
    glm::vec3 lightDiffuseColor = scene.GetDirLight().GetDiffuseCol();

    float sz                    = 240;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::SetNextWindowSize(ImVec2(sz, (float)height), ImGuiCond_Once);
    ImGui::SetNextWindowPos(ImVec2((float)width - sz, 0), ImGuiCond_Once);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.f, 0.f, 0.f, 0.7f));

    ImGui::Begin("##giz", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);

    ImGui::SetCursorPos(ImVec2(0, 0));
    ImGui::PushItemWidth(sz * 0.25f);
    bool quatChanged = false;
    if (pObject)
    {
        const ImVec4 oldTex(style.Colors[ImGuiCol_Text]);
        setTextCol(1, 0, 0);
        quatChanged |= ImGui::DragFloat("##u0", (float*)&objQua.x, 0.01f, -1.0, 1.0, "x: %.2f", 1.f);
        ImGui::SameLine();
        setTextCol(0, 1, 0);
        quatChanged |= ImGui::DragFloat("##u1", (float*)&objQua.y, 0.01f, -1.0, 1.0, "y: %.2f", 1.f);
        ImGui::SameLine();
        setTextCol(0, 0, 1);
        quatChanged |= ImGui::DragFloat("##u2", (float*)&objQua.z, 0.01f, -1.0, 1.0, "z: %.2f", 1.f);
        ImGui::SameLine();
        setTextCol(oldTex);
        quatChanged |= ImGui::DragFloat("##u3", (float*)&objQua.w, 0.01f, -1.0, 1.0, "w: %.2f", 1.f);
        ImGui::PopItemWidth();
    }

    bool doUpdateObj    = quatChanged;
    bool doUpdateCamera = false;
    bool doUpdateLight  = false;

    if (ImGui::DragFloat3("Light", value_ptr(lightDirWS), 0.01f)) { doUpdateLight = true; }

    if (pObject && ImGui::gizmo3D("Object", objPos, objQua, sz)) { doUpdateObj = true; }

    sz *= 0.5f;

    if (ImGui::gizmo3D("Camera", posin2, qtin2, sz)) { doUpdateCamera = true; }

    ImGui::SameLine();
    glm::vec3 vL(-lightDirWS);
    if (ImGui::gizmo3D("Light", vL, sz, imguiGizmo::sphereAtOrigin))
    {
        doUpdateLight = true;
        lightDirWS    = -vL;
    }
    ImVec4 color      = ImVec4(lightDiffuseColor[0], lightDiffuseColor[1], lightDiffuseColor[2], 1.00f);
    bool colorChanged = ImGui::ColorEdit3("clear color", (float*)&color);

    if (doUpdateObj)
    {
        // move from the camera to the world view
        xformObj = ge::Transform();
        xformObj.Translate(objPos);
        xformObj.RotateByQuat(glm::normalize(glm::inverse(qtin2) * objQua));
    }

    if (doUpdateCamera)
    {
        xformCam = ge::Transform();
        xformCam.Translate(posin2);
        xformCam.RotateByQuat(glm::normalize(qtin2));
    }

    if (doUpdateLight)
    {
        // move from the camera to the world view
#ifdef USE_LIGHT_POV_FOR_LIGHT
        lightDirWS = glm::normalize(glm::inverse(qtin2) * lightDirWS);
#else
        lightDirWS = glm::normalize(lightDirWS);
#endif
        scene.GetDirLight().SetDirWS(lightDirWS);
    }

    if (colorChanged) { scene.GetDirLight().SetDiffuseCol(glm::vec3(color.x, color.y, color.z)); }

    ImGui::SetCursorPos(ImVec2(0, (float)height - ImGui::GetFrameHeightWithSpacing() * 15));
    if (h_visible)
    {
        ImGui::TextColored(ImVec4(0.f, 1.f, 1.f, 1.f), "                    Help");
        ImGui::NewLine();
        ImGui::TextColored(ImVec4(0.f, 1.f, 0.f, 1.f), "Object and Camera rotations:");
        ImGui::Text("- Shft+btn -> Zoom");
        ImGui::Text("- Wheel    -> Zoom");
        ImGui::Text("- Ctrl+btn -> Pan/Move");
        ImGui::NewLine();
    }

    ImGui::SetCursorPos(ImVec2(0, (float)height - ImGui::GetFrameHeightWithSpacing()));
    if (ImGui::Button(" -= Show / Hide Help =- ")) h_visible ^= 1;
    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(4);
}

void ImguizmoDialog::SetVisibility(bool v)
{
    d_visible = v;
}

void ImguizmoDialog::ToggleVisibility()
{
    d_visible ^= 1;
}
