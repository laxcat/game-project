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
        .keyInput = keyInput,
        .mousePosInput = mousePosInput,
        .scrollInput = scrollInput,
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
    using input_callback_t = void (*)(Input const & input);

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
    input_callback_t keyInput = nullptr;
    input_callback_t charInput = nullptr;
    input_callback_t mousePosInput = nullptr;
    input_callback_t mouseButtonInput = nullptr;
    input_callback_t scrollInput = nullptr;

    Args args;

    WindowLimits windowLimits;

    // size in bytes of total memory
    size_t memManSize = 1024*1024*500;

    // size in bytes for frame-stack
    size_t memManFrameStackSize = 1024*1024*10;

    // number of concurrent auto-released allocations
    size_t memManAutoReleaseBufferSize = 32;

    // fixed-size allocator setup
    MemManFSASetup memManFSA{
        .n2byteSubBlocks = 32,
        .n4byteSubBlocks = 8,
        .n8byteSubBlocks = 8,
        .n16byteSubBlocks = 32,
        .n32byteSubBlocks = 32,
        .n64byteSubBlocks = 8,
        .n128byteSubBlocks = 32,
        .n256byteSubBlocks = 64,
        .n512byteSubBlocks = 8,
        .align = 64,
    };

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
