#pragma once
#include <fstream>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <glm/ext/matrix_transform.hpp>
#include "engine.h"
#include "common/glfw.h"
#include "common/MemorySystem.h"
#include "common/utils.h"
#include "render/Camera.h"
#include "render/CameraControl.h"
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
    size2 windowSize = {0, 0};
    GLFWwindow * window = nullptr;
    bgfx::ViewId mainView = 0;
    bgfx::ViewId guiView  = 0xff;
    double thisTime;
    double prevTime;
    double dt;
    char const * assetsPath = "";
    EngineSetup setup;

    MemorySystem memSys;
    RenderSystem rendSys;

    bool mouseIsDown = false;
    glm::vec2 mousePos;
    glm::vec2 mousePrevPos;
    glm::vec2 mouseDownPos;
    glm::vec2 mouseUpPos;

    Camera camera;
    CameraControl cameraControl;

    Renderable * originWidget;

    #if DEV_INTERFACE
    Editor editor;
    DevOverlay devOverlay;
    #endif // DEV_INTERFACE


// -------------------------------------------------------------------------- //
// LIFECYCLE
// -------------------------------------------------------------------------- //
    int init(EngineSetup const & setup) {
        assert(window && "set window");

        this->setup = setup;

        assetsPath = setup.assetsPath;

        thisTime = setup.startTime;
        prevTime = setup.startTime;

        int err = 0;
        if (setup.preInit) err = setup.preInit(setup.args);
        if (err) return err;

        memSys.init();
        rendSys.init();
        camera.init(windowSize);

        #if DEV_INTERFACE
        devOverlay.init(windowSize);
        devOverlay.setState(DevOverlay::None);
        devOverlay.showKeyboardShortcuts();
        #endif // DEV_INTERFACE

        #if DEV_INTERFACE
        originWidget = OriginWidget::create();
        originWidget->instances[0].active = false;
        OriginWidget::setScale(5.f);
        #endif // DEV_INTERFACE

        if (setup.postInit) err = setup.postInit(setup.args);
        if (err) return err;

        return 0;
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
        draw();
        if (setup.postDraw) setup.postDraw();
    }

    void draw() {
        bgfx::setViewClear(mainView, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH, rendSys.colors.background);
        bgfx::setViewTransform(mainView, (float *)&camera.viewMat, (float *)&camera.projMat);
        bgfx::setViewRect(mainView, 0, 0, bgfx::BackbufferRatio::Equal);
        rendSys.draw();
        bgfx::frame();
    }

    void updateSize(size2 windowSize) {
        if (setup.preResize) setup.preResize();

        this->windowSize = windowSize;
        rendSys.settings.updateSize(windowSize);
        camera.setRatio(windowSize);

        if (setup.postResize) setup.postResize();
    }


// -------------------------------------------------------------------------- //
// EVENT
// -------------------------------------------------------------------------- //

    void keyEvent(Event const & e) {
        if (e.action == GLFW_PRESS) {
            #if DEV_INTERFACE
            int numKey = glfwNumberKey(e.key);
            if (numKey != -1) {
                devOverlay.setState(numKey);
            }
            #endif // DEV_INTERFACE
        }
        if (setup.keyEvent) setup.keyEvent(e);
    }

    void mousePosEvent(Event const & e) {
        mousePrevPos = mousePos;
        mousePos.x = e.x;
        mousePos.y = e.y;
        if (setup.cameraControl) cameraControl.mousePosEvent(e);
        if (setup.mousePosEvent) setup.mousePosEvent(e);
    }

    void mouseButtonEvent(Event const & e) {
        mouseIsDown = e.action;
        if (mouseIsDown) {
            double tx, ty;
            glfwGetCursorPos(window, &tx, &ty);
            mousePos.x = tx;
            mousePos.y = ty;
            mouseDownPos = mousePos;
        }
        else {
            mouseUpPos = mousePos;
        }
        if (setup.mouseButtonEvent) setup.mouseButtonEvent(e);
    }

    void scrollEvent(Event const & e) {
        if (setup.cameraControl) cameraControl.scrollEvent(e);
        if (setup.scrollEvent) setup.scrollEvent(e);
    }
};


inline MrManager mm;
