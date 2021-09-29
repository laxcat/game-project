#include <stdio.h>

// #include <OpenGL/gl3.h>

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_COCOA
#include "GLFW/glfw3native.h"

#include "bgfx/bgfx.h"
#include "bgfx/platform.h"

#include "MrManager.h"


MrManager mm;


static void glfw_errorCallback(int error, const char *description) {
    fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

static void glfw_keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    mm.glfwEvent(window, key, scancode, action, mods);
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
    double time;

    mm.init();

    while (!glfwWindowShouldClose(window)) {
        time = (double)glfwGetTime();
        glfwPollEvents();
        mm.tick(time);
        bgfx::touch(kClearView);
        bgfx::frame();
    }

    // Exit
    glfwDestroyWindow(window);
    glfwTerminate();
    bgfx::shutdown();
    return 0;
}
