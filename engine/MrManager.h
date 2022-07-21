#pragma once
#include <fstream>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <glm/ext/matrix_transform.hpp>
#include "engine.h"
#include "common/glfw.h"
#include "common/InputSys.h"
#include "common/MemorySystem.h"
#include "common/utils.h"
#include "render/Camera.h"
#include "render/RenderSystem.h"

#if DEV_INTERFACE
#include "dev/DevOverlay.h"
#include "dev/Editor.h"
#include "dev/OriginWidget.h"
#endif // DEV_INTERFACE


// Central app manager. Unenforced singleton.
// Access through mm, an inline global variable found in this header. (inline var requires C++17)
// https://www.youtube.com/watch?v=ZdGrC9S4PYA

class MrManager {
public:
// -------------------------------------------------------------------------- //
// STATE VARS
// -------------------------------------------------------------------------- //
    size2 windowSize = {1920, 1080};
    bgfx::ViewId mainView = 0;
    bgfx::ViewId guiView  = 0xff;
    double thisTime;
    double prevTime;
    double dt;
    char const * assetsPath = "";
    EngineSetup setup;

    MemorySystem memSys;
    RenderSystem rendSys;
    InputSys input;

    Camera camera;

    Renderable * originWidget;

    #if DEV_INTERFACE
    Editor editor;
    DevOverlay devOverlay;
    #endif // DEV_INTERFACE


// -------------------------------------------------------------------------- //
// LIFECYCLE
// -------------------------------------------------------------------------- //
    void init(EngineSetup const & setup) {
        this->setup = setup;

        assetsPath = setup.assetsPath;

        thisTime = setup.startTime;
        prevTime = setup.startTime;

        if (setup.preInit) setup.preInit();

        memSys.init();
        rendSys.init();
        camera.init(windowSize);

        #if DEV_INTERFACE
        devOverlay.init(windowSize);
        devOverlay.setState(DevOverlay::DearImGUI);
        devOverlay.showKeyboardShortcuts();
        #endif // DEV_INTERFACE

        #if DEV_INTERFACE
        originWidget = OriginWidget::create();
        originWidget->instances[0].active = false;
        OriginWidget::setScale(5.f);
        #endif // DEV_INTERFACE

        if (setup.postInit) setup.postInit();
    }

    void shutdown() {
        if (setup.preShutdown) setup.preShutdown();
        memSys.shutdown();
        rendSys.shutdown();
        camera.shutdown();
        if (setup.postShutdown) setup.postShutdown();
    }

    void updateTime(double nowInSeconds) {
        prevTime = thisTime;
        thisTime = nowInSeconds;
        dt = thisTime - prevTime;
    }

    void tick() {
        if (setup.preDraw) setup.preDraw();

        // draw
        bgfx::setViewClear(mainView, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH, rendSys.colors.background);
        bgfx::setViewTransform(mainView, (float *)&camera.viewMat, (float *)&camera.projMat);
        bgfx::setViewRect(mainView, 0, 0, bgfx::BackbufferRatio::Equal);
        rendSys.draw();

        if (setup.postDraw) setup.postDraw();
    }

    void updateSize(size2 window) {
        if (setup.preResize) setup.preResize();

        windowSize = window;
        rendSys.settings.updateSize(window);
        camera.setRatio(window);

        if (setup.postResize) setup.postResize();
    }


// -------------------------------------------------------------------------- //
// EVENT
// -------------------------------------------------------------------------- //

    void keyEvent(int key, int scancode, int action, int mods) {
        if (action == GLFW_PRESS) {
            #if DEV_INTERFACE
            int numKey = glfwNumberKey(key);
            if (numKey != -1) {
                devOverlay.setState(numKey);
            }
            #endif // DEV_INTERFACE
        }
    }

    void mousePosEvent(double x, double y) {
        input.mousePosEvent(x, y);
        // fprintf(stderr, "mouse pos event %f %f\n", x, y);
    }

    void mouseButtonEvent(int button, int action, int mods) {
        input.mouseButtonEvent(button, action, mods);
        // fprintf(stderr, "mouse button event %d %d %d\n", button, action, mods);
    }

    void scrollEvent(double x, double y) {
        input.scrollEvent(x, y);
        // fprintf(stderr, "scroll event %f %f\n", x, y);
    }
};


inline MrManager mm;
