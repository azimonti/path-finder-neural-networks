/************************/
/*    gl_app.cpp        */
/*    Version 1.0       */
/*     2022/06/24       */
/************************/

#include <cstdio>
#include "log/log.h"
#include "dglutils.h"
#include "gl_app.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include "uibase.h"

static GLApp* getGLApp(GLFWwindow* pWin)
{
    return (GLApp*)glfwGetWindowUserPointer(pWin);
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
    LOGGER(logging::ERROR)
        << std::string("Glfw Error :").append(std::to_string(error)).append(": ").append(description);
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (ImGui::GetIO().WantCaptureKeyboard) return;

    GLApp* w = getGLApp(window);
    if (key == GLFW_KEY_PRINT_SCREEN && action == GLFW_PRESS)
    {
        if (scancode & GLFW_MOD_CONTROL) {} // CTRL+PrtScr -> request FileName
        if (!scancode) {}                   // CTRL+PrtScr -> TimeBased FileName
    }
#if 0
    else if(key >= GLFW_KEY_F1 && key <= GLFW_KEY_F12 && action == GLFW_PRESS)
    {
        w->GetEngineWnd()->onSpecialKeyDown(key, 0, 0);
    }
#endif
    else if (action == GLFW_PRESS || action == GLFW_RELEASE)
    {
        w->GetEngineWnd()->onKeyPress(key == GLFW_KEY_ENTER ? 13 : key, action == GLFW_PRESS);
    }
    else if ((GLFW_MOD_CONTROL == mods) && ('I' == key))
    {
        LOGGER(logging::INFO) << "CTRL i - Toggle imgui dialog";
        w->GetImguizmoDlg()->ToggleVisibility();
    }
    else if ((GLFW_MOD_CONTROL == mods) && ('Q' == key))
    {
        LOGGER(logging::INFO) << "CTRL q - exit";
        w->SetWindowShouldClose(true);
    }
    else {}
}

static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    double x, y;
    (void)mods;
    glfwGetCursorPos(window, &x, &y);

    getGLApp(window)->GetEngineWnd()->onMouseButton(button, action == GLFW_PRESS, x, y);
}

static void scrollCallback(GLFWwindow* window, double x, double y)
{
    getGLApp(window)->GetEngineWnd()->onScroll(x, y);
}

static void windowSizeCallback(GLFWwindow* window, int width, int height)
{
    getGLApp(window)->GetEngineWnd()->onReshape(width, height);
}

static void cursorPosCallback(GLFWwindow* window, double x, double y)
{
    getGLApp(window)->GetEngineWnd()->onMotion(x, y);
}

GLApp::GLApp()
{
    moEngineWin = std::make_unique<GLWindow>();
    moGizmoWin  = std::make_unique<ImguizmoDialog>();
}

GLApp::~GLApp()
{
    onExit();
}

void GLApp::onInit()
{
    // some day modify so that it uses loaded size (and pos ?)
    frameInit(WIN_TITLE, DEF_WIDTH, DEF_HEIGHT);
    imguiInit();
    moEngineWin->onInit();
    // moGizmoWin->SetVisibility(true);
}

void GLApp::frameInit(const char* pWinTitle, int winW, int winH)
{
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) exit(EXIT_FAILURE);
#if defined(__APPLE__)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif
    glfwWindowHint(GLFW_SAMPLES, 8);

    // determine content scale
#if !defined(__APPLE__)
    {
        int monsN     = 0;
        auto** ppMons = glfwGetMonitors(&monsN);
        for (int i = 0; i < monsN; ++i)
        {
            float x{};
            float y{};
            glfwGetMonitorContentScale(ppMons[i], &x, &y);
            // get the first available reasonable content scale
            if (x > 0 && y > 0)
            {
                mContentScale = x;
                break;
            }
        }
    }
#endif

    // set the coneent scale to UI Base's global for easy sharing
    UIB_ContentSca     = mContentScale;

    const auto useWinW = (int)((float)winW * mContentScale);
    const auto useWinH = (int)((float)winH * mContentScale);

    mpGLFWWin          = glfwCreateWindow(useWinW, useWinH, pWinTitle, NULL, NULL);
    if (!mpGLFWWin)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwMakeContextCurrent(mpGLFWWin);
    glfwSetWindowUserPointer(mpGLFWWin, (void*)this);
    glfwSetKeyCallback(mpGLFWWin, keyCallback);
    glfwSetMouseButtonCallback(mpGLFWWin, mouseButtonCallback);
    glfwSetScrollCallback(mpGLFWWin, scrollCallback);
    glfwSetWindowSizeCallback(mpGLFWWin, windowSizeCallback);
    glfwSetCursorPosCallback(mpGLFWWin, cursorPosCallback);

    // manually call it once to have the size set
    windowSizeCallback(mpGLFWWin, useWinW, useWinH);

    glfwSwapInterval(1);

    if (glewInit() != GLEW_OK)
    {
        fprintf(stderr, "Glew Error.\n");
        LOGGER(logging::ERROR) << std::string("Glew Error.");
    }

    // start intercepting and reporting GL errors without explicit checks
    // ...probably only in Windows
    DGLUT::SetupErrorIntercept();
}

void GLApp::imguiInit()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.IniFilename     = NULL;
    io.FontGlobalScale = mContentScale;

    // ImGui::StyleColorsClassic();
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    ImGui_ImplGlfw_InitForOpenGL(mpGLFWWin, true);
#if defined(__APPLE__)
    ImGui_ImplOpenGL3_Init("#version 150");
#else
    ImGui_ImplOpenGL3_Init("#version 130");
#endif

    ImPlot::CreateContext();
}

void GLApp::newFrame()
{
    glfwMakeContextCurrent(mpGLFWWin);

    int display_w{};
    int display_h{};
    glfwGetFramebufferSize(mpGLFWWin, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    moEngineWin->onRender(mpGLFWWin);
    moGizmoWin->onRender(mpGLFWWin);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(mpGLFWWin);
}

void GLApp::onExit()
{
    moEngineWin->onExit();
    imguiExit();
    frameExit();
}

void GLApp::imguiExit()
{
    ImPlot::DestroyContext();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void GLApp::frameExit()
{
    glfwDestroyWindow(mpGLFWWin);
    glfwTerminate();
}

void GLApp::mainLoop()
{
    while (!glfwWindowShouldClose(mpGLFWWin) && !mWindowShouldClose)
    {
        glfwPollEvents();

        moEngineWin->onAnim();

        newFrame();
    }
}
