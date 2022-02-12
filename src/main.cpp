#include <stdio.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>

#include "bgfx/bgfx.h"
#include "bgfx/platform.h"
#include "bgfx-imgui/imgui_impl_bgfx.h"

#include <backends/imgui_impl_glfw.h>

#include "MrManager.h"


static void glfw_errorCallback(int error, const char *description) {
    print("GLFW error %d: %s\n", error, description);
}

static void glfw_keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    mm.keyEvent(key, scancode, action, mods);
}

static void glfw_mousePosCallback(GLFWwindow *, double x, double y) {
    auto & io = ImGui::GetIO();
    io.MousePos.x = x;
    io.MousePos.y = y;
    mm.mousePosEvent(x, y);
}

static void glfw_mouseButtonCallback(GLFWwindow * window, int button, int action, int mods) {
    mm.mouseButtonEvent(button, action, mods);
}

static void glfw_scrollCallback(GLFWwindow * window, double x, double y) {
}


int main(int argc, char ** argv) {
    // glfw init, no graphics context!
    glfwSetErrorCallback(glfw_errorCallback);
    if (!glfwInit())
        return 1;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(mm.WindowSize.w, mm.WindowSize.h, "Window", nullptr, nullptr);
    if (!window) {
        print("window creation failed\n");
        return 1;
    }
    glfwSetKeyCallback(window, glfw_keyCallback);
    glfwSetCursorPosCallback(window, glfw_mousePosCallback);
    glfwSetMouseButtonCallback(window, glfw_mouseButtonCallback);
    glfwSetScrollCallback(window, glfw_scrollCallback);

    // ensure SINGLE THREAD
    bgfx::renderFrame();

    // bgfx init
    bgfx::Init init;
    init.platformData.nwh = glfwGetCocoaWindow(window);
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
    ImGui_ImplGlfw_InitForOther(window, true);

    while (!glfwWindowShouldClose(window)) {
        mm.prevTime = mm.thisTime;
        mm.thisTime = glfwGetTime();

        glfwPollEvents();

        if (mm.showImGUI) {
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
    mm.shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    bgfx::shutdown();
    return 0;
}
