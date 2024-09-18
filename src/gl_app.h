#ifndef _GLAPP_H_7651193BF92646A696068C609098E4D4_
#define _GLAPP_H_7651193BF92646A696068C609098E4D4_

/************************/
/*     gl_app.h         */
/*    Version 1.0       */
/*     2022/06/24       */
/*  © Davide Pasca      */
/*  © Marco Azimonti    */
/************************/

#include <memory>
#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "gl_window.h"
#include "imguizmo_dialog.h"

class GLApp
{
    public:
        GLApp();
        ~GLApp();

        void onInit();
        void onExit();
        void mainLoop();
        void newFrame();

        GLWindow* GetEngineWnd()         const { return moEngineWin.get(); }
        ImguizmoDialog* GetImguizmoDlg() const { return moGizmoWin.get();  }

        void SetWindowShouldClose(bool v) { mWindowShouldClose = v; }

        float GetContentScale() const { return mContentScale; }

    private:
        void frameInit( const char *pWinTitle, int winW, int winH );
        void frameExit();
        void imguiInit();
        void imguiExit();

        // defaults
        static constexpr int    DEF_WIDTH  = 1280;
        static constexpr int    DEF_HEIGHT = 768;
        // window name
        static constexpr const char *const WIN_TITLE = "Path Finder Neural Networks";

        bool        mWindowShouldClose  {};
        GLFWwindow  *mpGLFWWin          {};
        std::unique_ptr<GLWindow>           moEngineWin;
        std::unique_ptr<ImguizmoDialog>     moGizmoWin;
        float                               mContentScale {1.0f};
};

#endif
