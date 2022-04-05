#include <stdio.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>

#include "bgfx/bgfx.h"
#include "bgfx/platform.h"
#include "imgui_impl_bgfx.h"

#include <backends/imgui_impl_glfw.h>

#include "MrManager.h"


static void glfw_errorCallback(int error, const char *description) {
    fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

static void glfw_keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (!ImGui::GetIO().WantCaptureKeyboard || !mm.devOverlay.isShowingImGUI()) {
        mm.keyEvent(key, scancode, action, mods);
    }
}

static void glfw_mousePosCallback(GLFWwindow *, double x, double y) {
    if (!ImGui::GetIO().WantCaptureMouse || !mm.devOverlay.isShowingImGUI()) {
        mm.mousePosEvent(x, y);
    }
}

static void glfw_mouseButtonCallback(GLFWwindow * window, int button, int action, int mods) {
    if (!ImGui::GetIO().WantCaptureMouse || !mm.devOverlay.isShowingImGUI()) {
        mm.mouseButtonEvent(button, action, mods);
    }
}

static void glfw_scrollCallback(GLFWwindow * window, double x, double y) {
    if (!ImGui::GetIO().WantCaptureMouse || !mm.devOverlay.isShowingImGUI()) {
        mm.scrollEvent(x, y);
    }
}


int main(int argc, char ** argv) {
    // glfw init, no graphics context!
    glfwSetErrorCallback(glfw_errorCallback);
    if (!glfwInit())
        return 1;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    mm.window = glfwCreateWindow(mm.WindowSize.w, mm.WindowSize.h, "Window", nullptr, nullptr);
    if (!mm.window) {
        fprintf(stderr, "window creation failed\n");
        return 1;
    }
    glfwSetKeyCallback(mm.window, glfw_keyCallback);
    glfwSetCursorPosCallback(mm.window, glfw_mousePosCallback);
    glfwSetMouseButtonCallback(mm.window, glfw_mouseButtonCallback);
    glfwSetScrollCallback(mm.window, glfw_scrollCallback);

    // ensure SINGLE THREAD
    bgfx::renderFrame();

    // bgfx init
    bgfx::Init init;
    init.platformData.nwh = glfwGetCocoaWindow(mm.window);
    init.resolution.width = (uint32_t)mm.WindowSize.w;
    init.resolution.height = (uint32_t)mm.WindowSize.h;
    init.resolution.reset = BGFX_RESET_VSYNC;
    if (!bgfx::init(init))
        return 1;

    // Get assets path
    char cwd[PATH_MAX];
    char assetsPath[PATH_MAX * 2];
    if(getcwd(cwd, sizeof cwd)) {
        snprintf(assetsPath, sizeof assetsPath, "%s/assets", cwd);
    }

    // setup everything
    mm.init(glfwGetTime());

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_Implbgfx_Init(mm.guiView);
    ImGui_ImplGlfw_InitForOther(mm.window, true);

    while (!glfwWindowShouldClose(mm.window)) {
        mm.prevTime = mm.thisTime;
        mm.thisTime = glfwGetTime();

        glfwPollEvents();

        if (mm.devOverlay.isShowingImGUI()) {
            ImGui_Implbgfx_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            mm.gui();
            ImGui::Render();
            ImGui_Implbgfx_RenderDrawLists(ImGui::GetDrawData());
        }

        mm.tick();

        bgfx::frame();
    }

    ImGui_ImplGlfw_Shutdown();
    ImGui_Implbgfx_Shutdown();
    mm.shutdown();
    glfwDestroyWindow(mm.window);
    glfwTerminate();
    bgfx::shutdown();
    return 0;
}
