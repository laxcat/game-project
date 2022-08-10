#pragma once

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

struct EngineSetup {
    using init_callback_t = int (*)(Args const & args);
    using callback_t = void (*)();
    using input_callback_t = void (*)(Event const & event);

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
};

int main_desktop(EngineSetup const & setup);
