#ifndef _IMGUIZMO_DIALOG_H_EE46AA08654A4312A39F484BBD2B8774_
#define _IMGUIZMO_DIALOG_H_EE46AA08654A4312A39F484BBD2B8774_

/**************************/
/*   imguizmo_dialog.h    */
/*      Version 1.0       */
/*       2022/06/24       */
/**************************/

#include <GL/glew.h>
#include <GLFW/glfw3.h>

class ImguizmoDialog
{
  public:
    ImguizmoDialog();
    ~ImguizmoDialog();

    void onRender(GLFWwindow* window);
    void SetVisibility(bool v);
    void ToggleVisibility();

  private:
    bool d_visible{};
    bool h_visible{};
};
#endif
