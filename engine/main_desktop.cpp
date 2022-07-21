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
    mm.keyEvent(key, scancode, action, mods);
}

static void glfw_mousePosCallback(GLFWwindow *, double x, double y) {
    #if DEV_INTERFACE
    if (!ImGui::GetIO().WantCaptureMouse || !mm.devOverlay.isShowingImGUI()) {
        mm.mousePosEvent(x, y);
    }
    #else
    mm.mousePosEvent(x, y);
    #endif // DEV_INTERFACE
}

static void glfw_mouseButtonCallback(GLFWwindow *, int button, int action, int mods) {
    #if DEV_INTERFACE
    if (!ImGui::GetIO().WantCaptureMouse || !mm.devOverlay.isShowingImGUI()) {
        mm.mouseButtonEvent(button, action, mods);
    }
    #else
    mm.mouseButtonEvent(button, action, mods);
    #endif // DEV_INTERFACE
}

static void glfw_scrollCallback(GLFWwindow *, double x, double y) {
    #if DEV_INTERFACE
    if (!ImGui::GetIO().WantCaptureMouse || !mm.devOverlay.isShowingImGUI()) {
        mm.scrollEvent(x, y);
    }
    #else
    mm.scrollEvent(x, y);
    #endif // DEV_INTERFACE
}

int main_desktop(EngineSetup const & setup) {
    // glfw init, no graphics context!
    glfwSetErrorCallback(glfw_errorCallback);
    if (!glfwInit())
        return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    if (setup.transparentFramebuffer) {
        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE); // can be enabled to test transparent bg
    }
    GLFWwindow * window = glfwCreateWindow(mm.windowSize.w, mm.windowSize.h, "3D IC, Desktop-prototype, BGFX", nullptr, nullptr);
    if (!window) {
        fprintf(stderr, "window creation failed\n");
        return 1;
    }
    glfwSetWindowSizeCallback(window, glfw_windowSizeCallback);
    // glfwSetWindowAspectRatio(window, mm.windowSize.w, mm.windowSize.h);
    glfwMakeContextCurrent(window);

    // input callbacks
    glfwSetKeyCallback(window, glfw_keyCallback);
    glfwSetCursorPosCallback(window, glfw_mousePosCallback);
    glfwSetMouseButtonCallback(window, glfw_mouseButtonCallback);
    glfwSetScrollCallback(window, glfw_scrollCallback);

    // init
    mm.init(setup);

    // setup ImGUI
    #if DEV_INTERFACE
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_Implbgfx_Init(mm.guiView);
    ImGui_ImplGlfw_InitForOther(window, true);
    #endif // DEV_INTERFACE

    while (!glfwWindowShouldClose(window)) {
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
        bgfx::frame();
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

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
