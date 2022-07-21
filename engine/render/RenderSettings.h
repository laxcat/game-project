#pragma once
#include <bgfx/bgfx.h>
#include "../dev/print.h"
#include "../common/platform.h"


// Class designed to hold all the deep render settings, especially those 
// required to initialize and "reset" bgfx, so that these settings could be 
// exposed to the user.

class RenderSettings {
public:
    struct User {
        int msaa = 4;
        bool vsync = true;
        bool maxAnisotropy = false;

        bool operator == (User const & other) {
            return (
                other.msaa == msaa &&
                other.vsync == vsync
            );
        }
        bool operator != (User const & other) {
            return (
                other.msaa != msaa ||
                other.vsync != vsync
            );
        }
    };
    User user;

    bgfx::Init bgfxInit;
    uint64_t state = 0;

    void init(size2 windowSize) {
        bgfxInit.type = bgfx::RendererType::Metal;
        bgfxInit.platformData.context = getGLContext();
        bgfxInit.resolution.width = (uint32_t)windowSize.w;
        bgfxInit.resolution.height = (uint32_t)windowSize.h;
        updateState();
    }

    void updateState() {
        bgfxInit.resolution.reset = 0;
        if (user.vsync)             bgfxInit.resolution.reset |= BGFX_RESET_VSYNC;
        if (user.msaa == 4)         bgfxInit.resolution.reset |= BGFX_RESET_MSAA_X4;
        else if (user.msaa == 8)    bgfxInit.resolution.reset |= BGFX_RESET_MSAA_X8;
        else if (user.msaa == 16)   bgfxInit.resolution.reset |= BGFX_RESET_MSAA_X16;
        if (user.maxAnisotropy)     bgfxInit.resolution.reset |= BGFX_RESET_MAXANISOTROPY;

        state = 0
            | BGFX_STATE_WRITE_MASK
            | BGFX_STATE_DEPTH_TEST_LESS
            | BGFX_STATE_BLEND_ALPHA
            // | BGFX_STATE_CULL_CW
        ;
        if (user.msaa) state |= BGFX_STATE_MSAA;

        // print("UPDATED!!!! msaa=%d, vsync=%s, state=%d, reset=%d\n", 
            // user.msaa, user.vsync?"true":"false", state, bgfxInit.resolution.reset);
    }

    void updateSize(size2 windowSize) {
        bgfxInit.resolution.width = (uint32_t)windowSize.w;
        bgfxInit.resolution.height = (uint32_t)windowSize.h;
        reset();
    }

    void reinit() {
        updateState();
        reset();
    }

    void reset() {
        bgfx::reset(bgfxInit.resolution.width, bgfxInit.resolution.height, bgfxInit.resolution.reset);
    }
};
