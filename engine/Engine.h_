#pragma once

struct EngineSetup {
    using callback_t = void (*)();

    callback_t preInit = nullptr;
    callback_t postInit = nullptr;
    callback_t preEditor = nullptr;
    callback_t postEditor = nullptr;
    callback_t preDraw = nullptr;
    callback_t postDraw = nullptr;
    callback_t preResize = nullptr;
    callback_t postResize = nullptr;
    callback_t preShutdown = nullptr;
    callback_t postShutdown = nullptr;

    bool transparentFramebuffer = false;

    double startTime = 0.0;
    char const * assetsPath = "assets/";
};

int main_desktop(EngineSetup const & setup);
