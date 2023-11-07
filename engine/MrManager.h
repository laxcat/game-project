#pragma once
#include <bgfx/bgfx.h>
#include "engine.h"
#include "common/InputQueue.h"
#include "memory/Array.h"
#include "memory/MemMan.h"
#include "memory/FrameStack.h"
#include "render/Camera.h"
#include "render/CameraControl.h"
#include "render/RenderSystem.h"
#include "worker/Worker.h"

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
    size2 framebufferSize = {0, 0};
    GLFWwindow * window = nullptr;
    bgfx::ViewId mainView = 0;
    bgfx::ViewId guiView  = 0xff;
    double thisTime;
    double prevTime;
    double dt;
    size_t frame = 0;
    EngineSetup setup;

    InputQueue inputQueue;

    MemMan memMan;
    FrameStack * frameStack = nullptr; // TODO: consider moving this into MemMan
    RenderSystem rendSys;

    Array<Worker *> * workers = nullptr;
    Array<WorkerGroup> * workerGroups = nullptr;

    bool mouseIsDown = false;
    glm::vec2 mousePos;
    glm::vec2 mousePrevPos;
    glm::vec2 mouseDownPos;
    glm::vec2 mouseUpPos;

    Camera * camera;
    Camera defaultCamera;
    CameraControl cameraControl;

    #if DEV_INTERFACE
    Renderable * originWidget;
    Editor editor;
    enum DevState {
        DEV_STATE_NONE,
        DEV_STATE_INTERFACE,
        DEV_STATE_STATS,
        DEV_STATE_TEXT,
        DEV_STATE_WIREFRAME
    };
    DevState devState;
    #endif // DEV_INTERFACE

// -------------------------------------------------------------------------- //
// LIFECYCLE
// -------------------------------------------------------------------------- //
    int init(EngineSetup const & setup);
    void shutdown();
    // void updateTime(double nowInSeconds);
    void beginFrame(double nowInSeconds);
    void endFrame();
    void tick();
    void draw();
    void updateSize(size2 windowSize);

// -------------------------------------------------------------------------- //
// WORKERS
// -------------------------------------------------------------------------- //
    Worker * createWorker(Worker::Fn const & task, void * group = nullptr);
    void setWorkerGroupOnComplete(void * group, Worker::Fn const & onComplete);
    void joinWorkers();
    uint16_t numberOfWorkersInGroup(void * group) const;

// -------------------------------------------------------------------------- //
// FRAME STACK UTILS
// -------------------------------------------------------------------------- //
    char * frameStr(size_t size);
    char * frameByteSizeStr(size_t byteSize);
    char * frameFormatStr(char const * format, ...);

// -------------------------------------------------------------------------- //
// INPUT/EVENT
// -------------------------------------------------------------------------- //
    void processInputs();
    void keyInput(Input const & i);
    void charInput(Input const & i);
    void mousePosInput(Input const & i);
    void mouseButtonInput(Input const & i);
    void scrollInput(Input const & i);

// -------------------------------------------------------------------------- //
// TEST/DEBUG
// -------------------------------------------------------------------------- //
    #if DEV_INTERFACE
    void setDevState(DevState value);
    void showDevStateKeyboardShortcuts();
    bool isShowingDevInterface();
    #endif // DEV_INTERFACE
    void test();
};
