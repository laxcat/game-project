#include <stdio.h>

#include "common/glfw.h"
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#if ENABLE_IMGUI
#include "common/imgui_bgfx_glfw/imgui_bgfx_glfw.h"
#endif // ENABLE_IMGUI

#include "MrManager.h"
#include "engine.h"

// dev interface enabled, so imgui will be also
#if DEV_INTERFACE
    #define SHOULD_IMGUI_CAPTURE_KEYBOARD (mm.devOverlay.isShowingImGUI() && ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureKeyboard)
    #define SHOULD_IMGUI_CAPTURE_MOUSE    (mm.devOverlay.isShowingImGUI() && ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse)
// just imgui but not dev interface (using imgui in some other way)
#elif ENABLE_IMGUI
    #define SHOULD_IMGUI_CAPTURE_KEYBOARD (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureKeyboard)
    #define SHOULD_IMGUI_CAPTURE_MOUSE    (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse)
// no imgui at all
#else
    #define SHOULD_IMGUI_CAPTURE_KEYBOARD (false)
    #define SHOULD_IMGUI_CAPTURE_MOUSE    (false)
#endif // DEV_INTERFACE

static void glfw_errorCallback(int error, char const * description) {
    fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

static void glfw_windowSizeCallback(GLFWwindow * window, int width, int height) {
    mm.updateSize({width, height});
    // printl("glfw_windowSizeCallback, %d, %d", width, height);
}

static void glfw_keyCallback(GLFWwindow * window, int key, int scancode, int action, int mods) {
    mm.eventQueue.push({
        .type = Event::Key,
        .key = key,
        .scancode = scancode,
        .action = action,
        .mods = mods,
        .consume = true,
    });
}

static void glfw_charCallback(GLFWwindow * window, unsigned int codepoint) {
    mm.eventQueue.push({
        .type = Event::Char,
        .codepoint = codepoint,
        .consume = true,
    });
}

static void glfw_mousePosCallback(GLFWwindow * window, double x, double y) {
    mm.eventQueue.push({
        .type = Event::MousePos,
        .x = (float)x,
        .y = (float)y,
        .consume = true,
    });
}

static void glfw_mouseButtonCallback(GLFWwindow * window, int button, int action, int mods) {
    mm.eventQueue.push({
        .type = Event::MouseButton,
        .button = button,
        .action = action,
        .mods = mods,
        .consume = true,
    });
}

static void glfw_scrollCallback(GLFWwindow * window, double x, double y) {
    mm.eventQueue.push({
        .type = Event::MouseScroll,
        .scrollx = (float)x,
        .scrolly = (float)y,
        .consume = true,
    });
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

    // window hints
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

    // create window
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
    glfwSetWindowAspectRatio(mm.window, setup.windowLimits.ratioNumer, setup.windowLimits.ratioDenom);
    glfwSetWindowSizeLimits(
        mm.window,
        setup.windowLimits.minw, setup.windowLimits.minh,
        setup.windowLimits.maxw, setup.windowLimits.maxh
    );
    if (setup.forceOpenGL) glfwMakeContextCurrent(mm.window);

    // window/input callbacks
    glfwSetWindowSizeCallback(mm.window, glfw_windowSizeCallback);
    glfwSetKeyCallback(mm.window, glfw_keyCallback);
    glfwSetCharCallback(mm.window, glfw_charCallback);
    glfwSetCursorPosCallback(mm.window, glfw_mousePosCallback);
    glfwSetMouseButtonCallback(mm.window, glfw_mouseButtonCallback);
    glfwSetScrollCallback(mm.window, glfw_scrollCallback);

    // pre init
    err = 0;
    if (setup.preInit) err = setup.preInit(setup.args);
    if (err) return err;

    // init MrManager
    err = mm.init(setup);
    if (err) return err;

    // get actual window size (might not match requested), and framebuffer size
    glfwGetWindowSize(mm.window, &mm.windowSize.w, &mm.windowSize.h);

    // setup ImGUI
    #if ENABLE_IMGUI
    imguiCreate(
        mm.window,
        mm.guiView,
        {(float)mm.windowSize.w, (float)mm.windowSize.h}
    );
    #endif // ENABLE_IMGUI

    err = 0;
    if (setup.postInit) err = setup.postInit(setup.args);
    if (err) return err;

    while (!glfwWindowShouldClose(mm.window)) {
        // printf("DEV_INTERFACE %d\n", DEV_INTERFACE);
        // printf("ENABLE_IMGUI %d\n", ENABLE_IMGUI);

        glfwPollEvents();

        // sets delta time (mm.dt)
        mm.beginFrame(glfwGetTime());

        #if ENABLE_IMGUI
            // might consume events
            imguiBeginFrame(mm.windowSize, mm.eventQueue, mm.dt);
            // handle what wasn't consumed
            mm.processEvents();

            #if DEV_INTERFACE
            if (mm.devOverlay.isShowingImGUI()) {
                if (mm.setup.preEditor) mm.setup.preEditor();
                mm.editor.tick();
                if (mm.setup.postEditor) mm.setup.postEditor();
            }
            #endif // DEV_INTERFACE

            mm.tick();
            imguiEndFrame();
            mm.draw();

        // not ENABLE_IMGUI
        #else
            mm.processEvents();
            mm.tick();
            mm.draw();

        #endif // ENABLE_IMGUI

        mm.endFrame();

    }

    #if ENABLE_IMGUI
    imguiDestroy();
    #if DEV_INTERFACE
    mm.devOverlay.shutdown();
    #endif // DEV_INTERFACE
    #endif // ENABLE_IMGUI

    mm.shutdown();
    bgfx::shutdown();

    glfwDestroyWindow(mm.window);
    glfwTerminate();
    return 0;
}

int main_desktop(EngineSetup && setup) {
    return main_desktop(setup);
}
