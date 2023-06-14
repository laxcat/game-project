#pragma once
#include "common/types.h"
#include "common/debug_defines.h"

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

struct EngineSetup {
    using setup_callback_t = int (*)(EngineSetup & setup);
    using init_callback_t = int (*)(Args const & args);
    using callback_t = void (*)();
    using input_callback_t = void (*)(Event const & event);

    setup_callback_t preWindow = nullptr;
    init_callback_t preInit = nullptr;
    init_callback_t postInit = nullptr;
    callback_t preEditor = nullptr;
    callback_t prependInsideEditor = nullptr;
    callback_t appendInsideEditor = nullptr;
    callback_t postEditor = nullptr;
    callback_t preDraw = nullptr;
    callback_t postDraw = nullptr;
    callback_t preResize = nullptr;
    callback_t postResize = nullptr;
    callback_t preShutdown = nullptr;
    callback_t postShutdown = nullptr;
    input_callback_t keyEvent = nullptr;
    input_callback_t charEvent = nullptr;
    input_callback_t mousePosEvent = nullptr;
    input_callback_t mouseButtonEvent = nullptr;
    input_callback_t scrollEvent = nullptr;

    Args args;

    WindowLimits windowLimits;

    size_t memManSize = 1024*1024*500;
    size_t memManFrameStackSize = 1024*1024*10;
    MemManFSASetup memManFSA{
        .n2byteSubBlocks = 8,
        .n4byteSubBlocks = 320,
        .n8byteSubBlocks = 16,
        .align = 64,
    };

    // size_t memSysSize = 1024*1024*500;
    // size_t frameStackSize = 1024*1024*10;

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
