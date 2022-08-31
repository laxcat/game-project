#pragma once
#include <bgfx/bgfx.h>
#include "engine.h"
#include "memory/MemSys.h"
#include "render/Camera.h"
#include "render/CameraControl.h"
#include "render/RenderSystem.h"

#if DEV_INTERFACE
#include "dev/DevOverlay.h"
#include "dev/Editor.h"
#endif // DEV_INTERFACE

// Central app manager. Unenforced singleton.
// https://www.youtube.com/watch?v=ZdGrC9S4PYA

// Don't instatiate MrManager. Use this global.
class MrManager;
extern MrManager mm;

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
    EngineSetup setup;

    MemSys memSys;
    MemSys::Stack * frameStack = nullptr;
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
    int init(EngineSetup const & setup);
    void shutdown();
    void updateTime(double nowInSeconds);
    void tick();
    void draw();
    void updateSize(size2 windowSize);

// -------------------------------------------------------------------------- //
// TEMP STRING
// -------------------------------------------------------------------------- //
    char * tempStr(size_t size);

// -------------------------------------------------------------------------- //
// EVENT
// -------------------------------------------------------------------------- //
    void keyEvent(Event const & e);
    void mousePosEvent(Event const & e);
    void mouseButtonEvent(Event const & e);
    void scrollEvent(Event const & e);
    void test();
};
