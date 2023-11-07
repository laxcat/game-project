#pragma once
#include <functional>
#include <bgfx/bgfx.h>
#include "../common/platform.h"
#include "../common/utils.h"
#include "../dev/print.h"


// Class designed to hold all the deep render settings, especially those 
// required to initialize and "reset" bgfx, so that these settings could be 
// exposed to the user.

class RenderSettings {
public:
    struct User {
        int msaa = 2;
        bool vsync = true;
        bool maxAnisotropy = false;
        bool drawSceneAABB = true;
        std::function<void(bool)> didChangeDrawSceneAABB = nullptr;
        float aabbColor[4] = {0.0f, 1.0f, 0.0f, 1.0f};

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
    int msaaTemp = 0;

    void init(GLFWwindow * glfwWindow, size2 windowSize, bool forceOpenGL = false) {
        if (forceOpenGL) {
            bgfxInit.type = bgfx::RendererType::OpenGL;
            bgfxInit.platformData.context = getGLContext();
        }
        else {
            bgfxInit.type = bgfx::RendererType::Metal;
            bgfxInit.platformData.nwh = getNativeWindow(glfwWindow);
        }
        bgfxInit.resolution.width = (uint32_t)windowSize.w;
        bgfxInit.resolution.height = (uint32_t)windowSize.h;
        updateState();
    }

    void updateState() {
        /*
        RE: Resolution!

        BGFX_CAPS_HIDPI is always false on mac (as of testing)
        BGFX_RESET_HIDPI either doesen't do anything or is only considered when resizing? (unclear)

        https://github.com/bkaradzic/bgfx/issues/2009

        ! ! ! ! ! ! !
        Generally expect to handle backing buffer resolution MANUALLY, OUTSIDE bgfx.
        ! ! ! ! ! ! !

        Unaddressed for now.

        */

        bgfxInit.resolution.reset = 0;
        if (user.vsync)             bgfxInit.resolution.reset |= BGFX_RESET_VSYNC;
        switch (user.msaa) {
        case  2: {                  bgfxInit.resolution.reset |= BGFX_RESET_MSAA_X2;  break; }
        case  4: {                  bgfxInit.resolution.reset |= BGFX_RESET_MSAA_X4;  break; }
        case  8: {                  bgfxInit.resolution.reset |= BGFX_RESET_MSAA_X8;  break; }
        case 16: {                  bgfxInit.resolution.reset |= BGFX_RESET_MSAA_X16; break; }
        }
        if (user.maxAnisotropy)     bgfxInit.resolution.reset |= BGFX_RESET_MAXANISOTROPY;

        state = 0
            | BGFX_STATE_WRITE_MASK
            | BGFX_STATE_DEPTH_TEST_LESS
            // | BGFX_STATE_CULL_CW
        ;
        if (user.msaa) {
            state |= BGFX_STATE_MSAA;
        }

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

    void toggleMSAA() {
        swap(user.msaa, msaaTemp);
        reinit();
    }
};
