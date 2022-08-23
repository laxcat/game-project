#include <stdio.h>

#include "common/glfw.h"
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#if DEV_INTERFACE
#include "imgui_impl_bgfx.h"
#include <backends/imgui_impl_glfw.h>
#endif // DEV_INTERFACE

#include "MrManager.h"
#include "engine.h"


static void glfw_errorCallback(int error, const char *description) {
    fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

static void glfw_windowSizeCallback(GLFWwindow *, int width, int height) {
    mm.updateSize({width, height});
}


static void glfw_keyCallback(GLFWwindow *, int key, int scancode, int action, int mods) {
    #if DEV_INTERFACE
    if (mm.devOverlay.isShowingImGUI() && ImGui::GetIO().WantCaptureKeyboard) {
        return;
    }
    #endif // DEV_INTERFACE
    mm.keyEvent({.key=key, .scancode=scancode, .action=action, .mods=mods});
}

static void glfw_mousePosCallback(GLFWwindow *, double x, double y) {
    #if DEV_INTERFACE
    if (!ImGui::GetIO().WantCaptureMouse || !mm.devOverlay.isShowingImGUI()) {
        mm.mousePosEvent({.x=x, .y=y});
    }
    #else
    mm.mousePosEvent({.x=x, .y=y});
    #endif // DEV_INTERFACE
}

static void glfw_mouseButtonCallback(GLFWwindow *, int button, int action, int mods) {
    #if DEV_INTERFACE
    if (!ImGui::GetIO().WantCaptureMouse || !mm.devOverlay.isShowingImGUI()) {
        mm.mouseButtonEvent({.button=button, .action=action, .mods=mods});
    }
    #else
    mm.mouseButtonEvent({.button=button, .action=action, .mods=mods});
    #endif // DEV_INTERFACE
}

static void glfw_scrollCallback(GLFWwindow *, double x, double y) {
    #if DEV_INTERFACE
    if (!ImGui::GetIO().WantCaptureMouse || !mm.devOverlay.isShowingImGUI()) {
        mm.scrollEvent({.x=x, .y=y});
    }
    #else
    mm.scrollEvent({.x=x, .y=y});
    #endif // DEV_INTERFACE
}

int main_desktop(EngineSetup & setup) {
    // glfw init
    glfwSetErrorCallback(glfw_errorCallback);
    if (!glfwInit())
        return 1;

    // pre-window creation hook
    int err = 0;
    if (setup.preWindow) err = setup.preWindow(setup);
    if (err) return err;

    // create window
    if (setup.forceOpenGL) {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    }
    else {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    if (setup.transparentFramebuffer) {
        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE); // can be enabled to test transparent bg
    }
    mm.window = glfwCreateWindow(
        setup.requestWindowPosition.w,
        setup.requestWindowPosition.h,
        setup.windowTitle,
        nullptr,
        nullptr
    );
    if (!mm.window) {
        fprintf(stderr, "mm.window creation failed\n");
        return 1;
    }
    glfwGetWindowSize(mm.window, &mm.windowSize.w, &mm.windowSize.h);
    if (setup.requestWindowPosition.x != GLFW_DONT_CARE && setup.requestWindowPosition.y != GLFW_DONT_CARE) {
        glfwSetWindowPos(mm.window, setup.requestWindowPosition.x, setup.requestWindowPosition.y);
    }
    glfwSetWindowSizeCallback(mm.window, glfw_windowSizeCallback);
    glfwSetWindowAspectRatio(mm.window, setup.windowLimits.ratioNumer, setup.windowLimits.ratioDenom);
    glfwSetWindowSizeLimits(
        mm.window,
        setup.windowLimits.minw, setup.windowLimits.minh,
        setup.windowLimits.maxw, setup.windowLimits.maxh
    );
    if (setup.forceOpenGL) glfwMakeContextCurrent(mm.window);

    // input callbacks
    glfwSetKeyCallback(mm.window, glfw_keyCallback);
    glfwSetCursorPosCallback(mm.window, glfw_mousePosCallback);
    glfwSetMouseButtonCallback(mm.window, glfw_mouseButtonCallback);
    glfwSetScrollCallback(mm.window, glfw_scrollCallback);

    // init MrManager
    err = mm.init(setup);
    if (err) return err;

    // setup ImGUI
    #if DEV_INTERFACE
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_Implbgfx_Init(mm.guiView);
    ImGui_ImplGlfw_InitForOther(mm.window, true);
    #endif // DEV_INTERFACE

    while (!glfwWindowShouldClose(mm.window)) {
        // printf("wut %d\n", DEV_INTERFACE);

        glfwPollEvents();

        #if DEV_INTERFACE
        if (mm.devOverlay.isShowingImGUI()) {
            ImGui_Implbgfx_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            mm.editor.tick();
            ImGui::Render();
            ImGui_Implbgfx_RenderDrawLists(ImGui::GetDrawData());
        }
        mm.devOverlay.tick();
        #endif // DEV_INTERFACE

        mm.updateTime(glfwGetTime());
        mm.tick();
    }

    #if DEV_INTERFACE
    ImGui_Implbgfx_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    #endif // DEV_INTERFACE

    mm.shutdown();
    bgfx::shutdown();

    #if DEV_INTERFACE
    mm.devOverlay.shutdown();
    #endif // DEV_INTERFACE

    glfwDestroyWindow(mm.window);
    glfwTerminate();
    return 0;
}

int main_desktop(EngineSetup && setup) {
    return main_desktop(setup);
}
