#ifndef _GL_WINDOW_H_4A3302DDC7D148A2BC186CA63DF05763_
#define _GL_WINDOW_H_4A3302DDC7D148A2BC186CA63DF05763_

/************************/
/*     gl_window.h      */
/*    Version 1.0       */
/*     2022/06/24       */
/************************/

#include <memory>
#include <glm/mat4x4.hpp>
#include "camera.h"

class CS_Scenario;
class CamManager;

namespace ge
{
    class Mesh;
    class Scene;
    class RendSetup;
} // namespace ge

class GLWindow
{
  public:
    GLWindow();
    virtual ~GLWindow();

    void onInit();
    void onExit();

    void onAnim();
    void onRender(GLFWwindow* window);
    void onReshape(GLint w, GLint h);

    const ge::Mesh* GetObjMesh() const { return nullptr; }

    ge::Mesh* GetObjMesh() { return nullptr; }

    const ge::Camera& GetCamera() const { return mCamera; }

    ge::Camera& GetCamera() { return mCamera; }

    const ge::Scene& GetScene() const { return *moScene; }

    ge::Scene& GetScene() { return *moScene; }

    void onMouseButton(int button, bool isDown, double x, double y);
    void onScroll(double x, double y);
    void onMotion(double x, double y);
    void onPassiveMotion(int x, int y);

    void onKeyPress(int key, bool isDown);
    void onSpecialKeyUp(int key, int x, int y);
    void onSpecialKeyDown(int key, int x, int y);

    bool makeRendSetup(ge::RendSetup& rsetup);

  private:
    int mWinWD{};
    int mWinHE{};

    std::unique_ptr<CamManager> moCamManager;

    ge::Camera mCamera;
    std::unique_ptr<ge::Scene> moScene;

    std::unique_ptr<CS_Scenario> moScenario;

    bool mFastFwdMode = false;
};

#endif
