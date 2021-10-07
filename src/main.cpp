#include <stdio.h>

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_COCOA
#include "GLFW/glfw3native.h"

#include "bgfx/bgfx.h"
#include "bgfx/platform.h"
#include "bgfx-imgui/imgui_impl_bgfx.h"

#include "MrManager.h"


MrManager mm;


static void glfw_errorCallback(int error, const char *description) {
    fprintf(stderr, "GLFW error %d: %s\n", error, description);
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
    auto & io = ImGui::GetIO();
    if (button == 0) io.MouseDown[0] = (action == GLFW_PRESS);
    if (button == 1) io.MouseDown[1] = (action == GLFW_PRESS);
    mm.mouseButtonEvent(button, action, mods);
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
        fprintf(stderr, "window creation failed\n");
        return 1;
    }
    glfwSetKeyCallback(window, glfw_keyCallback);
    glfwSetCursorPosCallback(window, glfw_mousePosCallback);
    glfwSetMouseButtonCallback(window, glfw_mouseButtonCallback);

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

    // Set view 0 to the same dimensions as the window and to clear the color buffer.
    const bgfx::ViewId kClearView = 0;
    bgfx::setViewClear(kClearView, BGFX_CLEAR_COLOR);
    bgfx::setViewRect(kClearView, 0, 0, bgfx::BackbufferRatio::Equal);

    mm.init(glfwGetTime());

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_Implbgfx_Init(kClearView);
    auto & io = ImGui::GetIO();
    io.DisplaySize = {(float)mm.WindowSize.w, (float)mm.WindowSize.h};
    io.IniFilename = NULL;

    while (!glfwWindowShouldClose(window)) {
        mm.prevTime = mm.thisTime;
        mm.thisTime = glfwGetTime();
        io.DeltaTime = mm.thisTime - mm.prevTime;

        glfwPollEvents();

        ImGui_Implbgfx_NewFrame();
        ImGui::NewFrame();

        mm.tick();

        ImGui::Render();
        ImGui_Implbgfx_RenderDrawLists(ImGui::GetDrawData());

        bgfx::touch(kClearView);
        bgfx::frame();
    }

    mm.shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    bgfx::shutdown();
    return 0;
}
