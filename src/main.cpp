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
    auto & io = ImGui::GetIO();
    io.KeyCtrl  = mods & GLFW_MOD_CONTROL;
    io.KeyShift = mods & GLFW_MOD_SHIFT;
    io.KeyAlt   = mods & GLFW_MOD_ALT;
    io.KeySuper = mods & GLFW_MOD_SUPER;
    io.KeysDown[key] = (action == GLFW_PRESS || action == GLFW_REPEAT);
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

static void glfw_scrollCallback(GLFWwindow * window, double x, double y) {
    auto & io = ImGui::GetIO();
    io.MouseWheel = y;
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
    auto & io = ImGui::GetIO();
    io.DisplaySize = {(float)mm.WindowSize.w, (float)mm.WindowSize.h};
    io.IniFilename = NULL;
    io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
    io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
    io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
    io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
    io.KeyMap[ImGuiKey_Insert] = GLFW_KEY_INSERT;
    io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Space] = GLFW_KEY_SPACE;
    io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
    io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
    io.KeyMap[ImGuiKey_KeyPadEnter] = GLFW_KEY_KP_ENTER;
    io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
    io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
    io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
    io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
    io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
    io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

    while (!glfwWindowShouldClose(window)) {
        mm.prevTime = mm.thisTime;
        mm.thisTime = glfwGetTime();
        io.DeltaTime = mm.thisTime - mm.prevTime;

        glfwPollEvents();

        if (mm.showImGUI) {
            ImGui_Implbgfx_NewFrame();
            ImGui::NewFrame();
            mm.gui();
            ImGui::Render();
            ImGui_Implbgfx_RenderDrawLists(ImGui::GetDrawData());
        }

        mm.tick();

        bgfx::frame();
    }

    mm.shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    bgfx::shutdown();
    return 0;
}
