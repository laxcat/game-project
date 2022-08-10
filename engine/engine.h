#pragma once

struct Args {
    int c = 0;
    char ** v = nullptr;
};

struct EngineSetup {
    using init_callback_t = int (*)(Args const & args);
    using callback_t = void (*)();

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

    Args args;

    bool transparentFramebuffer = false;

    bool forceOpenGL =
    #if FORCE_OPENGL
        true;
    #else
        false;
    #endif

    double startTime = 0.0;
    char const * assetsPath = "assets/";
};

int main_desktop(EngineSetup const & setup);
