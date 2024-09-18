/************************/
/*     gl_window.h      */
/*      main.cpp        */
/*    Version 1.0       */
/*     2022/06/24       */
/************************/

#include <algorithm>
#include <functional>
#include <stdexcept>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdarg.h>
#include "log/log.h"
#include "cammanager.h"
#include "cs_scenario.h"
#include "ge_mesh2.h"
#include "geomprocessing.h"
#include "gl_window.h"
#include "mesh.h"
#include "scene.h"
#include "uibase.h"

GLWindow::GLWindow()
{
    mCamera.GetTransform().SetMatrix(glm::mat4(1.f));
    mCamera.GetTransform().Translate(glm::vec3(0, -0.f, -100.f));
    mCamera.GetTransform().RotateByAxis(ge::ToRad(35.f), glm::vec3(1, 0, 0));
    // mCamera.GetTransform().RotateByAxis( ge::ToRad(-30.f), glm::vec3(0,1,0) );
}

GLWindow::~GLWindow() {}

void GLWindow::onInit()
{
    moScenario                    = std::make_unique<CS_Scenario>();

    moScene                       = std::make_unique<ge::Scene>();

    moScene->GetDirLight().mDirWS = glm::normalize(glm::vec3(0.1f, 0.6f, 0.3f));

    moCamManager                  = std::make_unique<CamManager>();
}

void GLWindow::onExit() {}

void GLWindow::onAnim()
{
    moCamManager->AnimateCM(mCamera.GetTransform());

    moScenario->AnimScenario(!mFastFwdMode);
}

bool GLWindow::makeRendSetup(ge::RendSetup& rsetup)
{
    if (!mWinWD || !mWinHE) return false;

    // camera projection setup
    mCamera.mFOV        = 60.f;
    mCamera.mAspect_woh = (float)mWinWD / (float)mWinHE;

    rsetup.mpCamera     = &mCamera;
    rsetup.mRTargetSiz  = ge::Vec2((float)mWinWD, (float)mWinHE);

    return true;
}

void GLWindow::onRender(GLFWwindow* window)
{
    (void)window;
    ge::RendSetup rsetup;
    if (!makeRendSetup(rsetup)) return;

    moScene->BeginRender(rsetup);
    moScenario->AddMeshesToScene(*moScene);

    moScenario->DrawScenarioUI(mCamera, moCamManager->mCamHand);

    moCamManager->DrawCamManagerUI(mWinWD, mWinHE);
}

void GLWindow::onReshape(GLint w, GLint h)
{
    mWinWD = w;
    mWinHE = h;
}

void GLWindow::onKeyPress(int key, bool isDown)
{
    // if ( !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) )
    if (key == GLFW_KEY_F1) { mFastFwdMode = isDown; }
    else { moCamManager->OnKeyPressCM(key, isDown); }
}

void GLWindow::onSpecialKeyDown(int key, int x, int y)
{
    (void)key;
    (void)x;
    (void)y;
}

void GLWindow::onSpecialKeyUp(int key, int x, int y)
{
    (void)key;
    (void)x;
    (void)y;
}

void GLWindow::onMouseButton(int button, bool isDown, double x, double y)
{
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) return;

    moCamManager->OnMouseButtonCM(mCamera.GetTransform(), button, isDown, x, y,
                                  [&](auto pickX, auto pickY) -> ge::ScenePickData {
        ge::RendSetup rsetup;
        if (!makeRendSetup(rsetup)) return {};

        // printf("** Picking (%i) %f %f\n", (int)doRotateCam, pickX, pickY );
        moScene->BeginPicking(rsetup, pickX, pickY);
        moScenario->AddMeshesToScene(*moScene);

        auto data = moScene->EndPicking();
        if (data.mpMesh) moScenario->OnPickedMeshSce(data.mpMesh);

        return data;
    });
}

void GLWindow::onScroll(double x, double y)
{
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) return;

    moCamManager->OnScrollCM(mCamera.GetTransform(), x, y);
}

void GLWindow::onMotion(double x, double y)
{
    moCamManager->OnMotionCM(mCamera.GetTransform(), x, y, mWinWD, mWinHE);
}

void GLWindow::onPassiveMotion(int x, int y)
{
    (void)x;
    (void)y;
}
