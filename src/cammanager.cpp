/************************/
/*    cammanager.cpp    */
/*    Version 1.0       */
/*     2022/07/07       */
/************************/

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "cammanager.h"
#include "ge_scenepicking.h"
#include "transform.h"
#include "uibase.h"

CamManager::CamManager()
{
    CH::CamHandMouse::Mappings par;
    par.km_arrowL = GLFW_KEY_LEFT;
    par.km_arrowR = GLFW_KEY_RIGHT;
    par.km_arrowU = GLFW_KEY_UP;
    par.km_arrowD = GLFW_KEY_DOWN;
    mCamHand.SetMappings(par);
}

void CamManager::DrawCamManagerUI(int rootWinW, int rootWinH)
{
    (void)rootWinH;
    ImGui::SetNextWindowPos({(float)rootWinW - 4, 4}, ImGuiCond_Always, {1.f, 0.f});

    if (!ImGui::Begin("Camera Manager", &mShoWin, 0))
    {
        ImGui::End();
        return;
    }

    auto doRadio = [&](const auto* pStr, auto val) {
        if (ImGui::RadioButton(pStr, mCamHand.GetCamMode() == val)) mCamHand.SetCamMode(val);
    };
    doRadio("Hook mode", CH::CamHandMouse::MODE_HOOK);
    ImGui::SameLine();
    doRadio("Fly mode", CH::CamHandMouse::MODE_FLY);

    ImGui::End();
}

void CamManager::OnMouseButtonCM(ge::Transform& camXF, int button, bool isDown, double x, double y,
                                 const std::function<ge::ScenePickData(float, float)>& pickFn)
{
    // printf( "Mouse button. button:%i isDown:%i x:%f y:%f\n", button, isDown, x, y );
    mCamHand.OnMouseButton(camXF.GetMatrix(), button, isDown, (float)x, (float)y);

    // did the user click somewhere to pick ?
    auto onPicking = [&](bool doRotateCam, auto pickMsg, auto pickX, auto pickY) {
        if (!pickMsg) return;

        const auto data = pickFn(pickX, pickY);
        // set the camera center in WS at the picking point
        if (!data.HasPicked()) return;

        // printf("Picked WS %f %f %f\n",
        //     data.mIntersWS[0],data.mIntersWS[1],data.mIntersWS[2]);
        mCamHand.SetPickPosWS(data.mIntersWS);

        if (doRotateCam)
        {
            const auto posWS = camXF.GetAffineInverse().GetTranslation();
            camXF            = glm::lookAt(posWS, data.mIntersWS, ge::Vec3(0, 1, 0));
        }
    };

    onPicking(false, mCamHand.GetPickMessage(), mCamHand.GetPickPointX(), mCamHand.GetPickPointY());

    onPicking(true, mCamHand.GetDClickMessage(), mCamHand.GetDClickPointX(), mCamHand.GetDClickPointY());
}

void CamManager::OnScrollCM(ge::Transform& camXF, double x, double y)
{
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) return;

    mCamHand.OnScroll(camXF.GetMatrix(), (float)x, (float)y);
}

void CamManager::OnMotionCM(ge::Transform& camXF, double x, double y, int winW, int winH)
{
    mCamHand.OnMouseMove(camXF.GetMatrix(), (float)x, (float)y, (float)winW, (float)winH);
}

void CamManager::OnKeyPressCM(int key, bool isDown)
{
    mCamHand.OnKeyPress(key, isDown);
}

void CamManager::AnimateCM(ge::Transform& camXF)
{
    mCamHand.AnimCamHand(camXF.GetMatrix());
}
