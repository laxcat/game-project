#pragma once

/*
Engine entry point.

Example usage:

```
int main(int argc, char ** argv) {
    return main_desktop({
        .args = {argc, argv},
        .preWindow = preWindow,
        .postInit = postInit,
        .preDraw = preDraw,
        .postResize = postResize,
        .preShutdown = preShutdown,
        .keyEvent = keyEvent,
        .mousePosEvent = mousePosEvent,
        .scrollEvent = scrollEvent,
        .windowTitle = "Example",
        .cameraControl = false,
        .limits = {.minw=200, .minh=200},
    });
}
```

*/


struct Args {
    int c = 0;
    char ** v = nullptr;
};

struct Event {
    int key = 0;
    int scancode = 0;
    int button = 0;
    int action = 0;
    int mods = 0;
    double x = 0.0;
    double y = 0.0;
};

struct Limits {
    // always passed directly into glfwSetWindowSizeLimits
    // these defaults match GLFW_DONT_CARE, which effectively means no constraints are set
    int minw = -1;
    int minh = -1;
    int maxw = -1;
    int maxh = -1;
    int ratioNumer = -1;
    int ratioDenom = -1;
};

struct WindowPlacement {
    int x = -1;
    int y = -1;
    int w = 1920;
    int h = 1080;
};

struct EngineSetup {
    using setup_callback_t = int (*)(EngineSetup & setup);
    using init_callback_t = int (*)(Args const & args);
    using callback_t = void (*)();
    using input_callback_t = void (*)(Event const & event);

    setup_callback_t preWindow = nullptr;
    init_callback_t preInit = nullptr;
    init_callback_t postInit = nullptr;
    callback_t preEditor = nullptr;
    callback_t postEditor = nullptr;
    callback_t preDraw = nullptr;
    callback_t postDraw = nullptr;
    callback_t preResize = nullptr;
    callback_t postResize = nullptr;
    callback_t preShutdown = nullptr;
    callback_t postShutdown = nullptr;
    input_callback_t keyEvent = nullptr;
    input_callback_t mousePosEvent = nullptr;
    input_callback_t mouseButtonEvent = nullptr;
    input_callback_t scrollEvent = nullptr;

    Args args;

    Limits limits;

    bool transparentFramebuffer = false;

    bool forceOpenGL =
    #if FORCE_OPENGL
        true;
    #else
        false;
    #endif

    bool cameraControl = true;

    double startTime = 0.0;
    char const * assetsPath = "assets/";

    char const * windowTitle = "Game Project Example";

    WindowPlacement requestWindowPosition;
};

int main_desktop(EngineSetup && setup);
int main_desktop(EngineSetup & setup);
