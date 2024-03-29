#include "MrManager.h"
#include "common/glfw.h"

MrManager mm;

int MrManager::init(EngineSetup const & setup) {
    assert(window && "set window");

    this->setup = setup;

    thisTime = setup.startTime;
    prevTime = setup.startTime;

    camera = &defaultCamera;

    memMan.init(setup, &frameStack);
    rendSys.init();
    camera->init(windowSize);
    editor.init();

    workers = memMan.createArray<Worker *>(64);
    workerGroups = memMan.createArray<WorkerGroup>(16);

    #if DEV_INTERFACE
    setDevState(DEV_STATE_INTERFACE);
    showDevStateKeyboardShortcuts();
    #endif // DEV_INTERFACE

    if (setup.args.c > 1) {
    }

    // test();

    createWorker([this]{
        // Gobj * g = memMan.createGobj("../../../gltf_assets/Box With Spaces/Box With Spaces.gltf");
        // Gobj * g = memMan.createGobj("../../../gltf_assets/Cameras.gltf");
        // Gobj * g = memMan.createGobj("../../../gltf_assets/CesiumMilkTruck.glb");
        // Gobj * g = memMan.createGobj("../../../gltf_assets/CesiumMilkTruck.glb");
        // Gobj * g = memMan.createGobj("../../../gltf_assets/CesiumMilkTruck/CesiumMilkTruck.gltf");
        // Gobj * g = memMan.createGobj("../../../gltf_assets/LS500.glb");
        // Gobj * g = memMan.createGobj("../../glTF-Sample-Models/2.0/BoomBox/glTF-Binary/BoomBox.glb");
        // Gobj * g = memMan.createGobj("../../glTF-Sample-Models/2.0/Box With Spaces/glTF/Box With Spaces.gltf");
        // Gobj * g = memMan.createGobj("../../glTF-Sample-Models/2.0/Box/glTF-Binary/Box.glb");
        // Gobj * g = memMan.createGobj("../../glTF-Sample-Models/2.0/BoxInterleaved/glTF-Binary/BoxInterleaved.glb");
        // Gobj * g = memMan.createGobj("../../glTF-Sample-Models/2.0/BoxTextured/glTF-Binary/BoxTextured.glb");
        // Gobj * g = memMan.createGobj("../../glTF-Sample-Models/2.0/BoxTextured/glTF-Embedded/BoxTextured.gltf");
        // Gobj * g = memMan.createGobj("../../glTF-Sample-Models/2.0/ClearCoatCarPaint/glTF-Binary/ClearCoatCarPaint.glb");
        // Gobj * g = memMan.createGobj("../../glTF-Sample-Models/2.0/DamagedHelmet/glTF-Binary/DamagedHelmet.glb");
        // Gobj * g = memMan.createGobj("../../glTF-Sample-Models/2.0/FlightHelmet/glTF/FlightHelmet.gltf");
        // Gobj * g = memMan.createGobj("../../glTF-Sample-Models/2.0/MaterialsVariantsShoe/glTF-Binary/MaterialsVariantsShoe.glb");
        // Gobj * g = memMan.createGobj("../../glTF-Sample-Models/2.0/MetalRoughSpheres/glTF-Binary/MetalRoughSpheres.glb");
        Gobj * g = memMan.createGobj("../../glTF-Sample-Models/2.0/MetalRoughSpheresNoTextures/glTF-Binary/MetalRoughSpheresNoTextures.glb");
        // Gobj * g = memMan.createGobj("../../glTF-Sample-Models/2.0/MorphStressTest/glTF-Binary/MorphStressTest.glb");
        // Gobj * g = memMan.createGobj("../../glTF-Sample-Models/2.0/MultipleScenes/glTF-Embedded/MultipleScenes.gltf");
        // Gobj * g = memMan.createGobj("../../glTF-Sample-Models/2.0/TextureCoordinateTest/glTF-Binary/TextureCoordinateTest.glb");
        // Gobj * g = memMan.createGobj("../../glTF-Sample-Models/2.0/TwoSidedPlane/glTF/TwoSidedPlane.gltf");

        // g = memMan.updateGobj(g, {.materials=1, .textures=1});
        // Gobj * g2 = memMan.createGobj(g->counts);
        // g2->copy(g);

        // pointers match?
        // printl("g %p, g2 %p", g, g2);
        // byte_t * gb  = (byte_t *)g;
        // byte_t * gb2 = (byte_t *)g2;
        // for (int i = 0; i < 23; ++i) {
        //     size_t offset = i * sizeof(void *);
        //     byte_t * p  = gb  + offset;
        //     byte_t * p2 = gb2 + offset;
        //     void * pp  = (void *)*(size_t *)p ;
        //     void * pp2 = (void *)*(size_t *)p2;
        //     ssize_t t  = (pp  == nullptr) ? 0 : (ssize_t)pp  - (ssize_t)(void *)gb;
        //     ssize_t t2 = (pp2 == nullptr) ? 0 : (ssize_t)pp2 - (ssize_t)(void *)gb2;

        //     printl("i: %2d, "
        //         "g [%3zu] (%p) = %11p, diff: %15zd, "
        //         "g2[%3zu] (%p) = %11p, diff: %15zd? "
        //         "%d",
        //         i,
        //         offset, p,  pp,  t,
        //         offset, p2, pp2, t2,
        //         t == t2
        //     );
        // }

        if (g) {
            rendSys.add("test", g);
        }
        // rendSys.remove("test");
    });

    return 0;
}

void MrManager::shutdown() {
    if (setup.preShutdown) setup.preShutdown();
    rendSys.shutdown();
    camera->shutdown();
    memMan.request({.ptr=workers, .size=0});
    memMan.request({.ptr=workerGroups, .size=0});
    if (setup.postShutdown) setup.postShutdown();
}

void MrManager::beginFrame(double nowInSeconds) {
    // update frame
    ++frame;
    memMan.startFrame(frame);
    // update time
    prevTime = thisTime;
    thisTime = nowInSeconds;
    dt = thisTime - prevTime;
    bgfx::dbgTextPrintf(0, 0, 0x0f, "DT:  %5.1fms", dt * 1000.0);
    bgfx::dbgTextPrintf(0, 1, 0x0e, "FPS: %5.1fms", 1.0 / dt);
}

void MrManager::endFrame() {
    if (frameStack) frameStack->reset();
    memMan.endFrame();
}

void MrManager::tick() {
    joinWorkers();
}

void MrManager::draw() {
    if (setup.preDraw) setup.preDraw();
    bgfx::setViewClear(mainView, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH, rendSys.colors.background.asRGBAInt());
    bgfx::setViewTransform(mainView, (float *)&camera->viewMat, (float *)&camera->projMat);
    bgfx::setViewRect(mainView, 0, 0, bgfx::BackbufferRatio::Equal);
    rendSys.draw();
    bgfx::frame();
    if (setup.postDraw) setup.postDraw();
}

void MrManager::updateSize(size2 windowSize) {
    if (setup.preResize) setup.preResize();

    // printl("updateSize: %d, %d", windowSize.w, windowSize.h);

    this->windowSize = windowSize;
    rendSys.settings.updateSize(windowSize);
    camera->setRatio(windowSize);

    if (setup.postResize) setup.postResize();
}

// -------------------------------------------------------------------------- //
// WORKERS
// -------------------------------------------------------------------------- //
    Worker * MrManager::createWorker(Worker::Fn const & task, void * group) {
        Worker * w = memMan.create<Worker>();
        w->init(task, group);
        workers->append(w);

        if (group) {
            size_t size = workerGroups->size();
            size_t i = 0;
            for (; i < size; ++i) {
                if (workerGroups->data()[i].id == group) break;
            }
            // not found, create
            if (i == size) {
                workerGroups->append({group, 1, nullptr});
            }
            // found, increment
            else {
                ++workerGroups->data()[i].count;
            }
        }

        return w;
    }

    void MrManager::setWorkerGroupOnComplete(void * group, Worker::Fn const & onComplete) {
        size_t size = workerGroups->size();
        size_t i = 0;
        for (; i < size; ++i) {
            if (workerGroups->data()[i].id == group) break;
        }
        // not found, create
        if (i == size) {
            workerGroups->append({group, 0, onComplete});
        }
        // found, replace
        else {
            workerGroups->data()[i].onComplete = onComplete;
        }
    }

    uint16_t MrManager::numberOfWorkersInGroup(void * group) const {
        if (workerGroups->size() == 0) {
            return 0;
        }
        WorkerGroup const * wg = workerGroups->find([group](WorkerGroup const & a){
            return (a.id == group);
        });
        if (wg == nullptr) {
            return 0;
        }
        return wg->count;
    }

    void MrManager::joinWorkers() {
        size_t size = workers->size();
        for (size_t i = 0; i < size; ++i) {
            Worker * w = workers->data()[i];
            if (w->isComplete()) {
                w->join();

                if (w->group()) {
                    size_t size = workerGroups->size();
                    size_t i = 0;
                    for (; i < size; ++i) {
                        if (workerGroups->data()[i].id == w->group()) break;
                    }
                    // found, decrement
                    if (i != size) {
                        WorkerGroup * wg = workerGroups->data() + i;
                        --wg->count;
                        if (wg->count == 0) {
                            if (wg->onComplete) {
                                wg->onComplete();
                            }
                            workerGroups->remove(i);
                        }
                    }
                }

                w->shutdown();
                memMan.request({.ptr=w, .size=0});
                workers->remove(i);
                --i;
                --size;
            }
        }
    }

// -------------------------------------------------------------------------- //
// FRAME STACK UTILS
// -------------------------------------------------------------------------- //

char * MrManager::frameStr(size_t size) {
    assert(frameStack && "Frame stack not initialized.");
    char * ret = frameStack->alloc<char>(size);
    assert(ret && "Could not allocate temp string.");
    return ret;
}

char * MrManager::frameByteSizeStr(size_t byteSize) {
    assert(frameStack && "Frame stack not initialized.");
    if (byteSize > 1024*1024) {
        return frameStack->formatStr("%.0f MB", round((double)byteSize/(1024*1024)));
    }
    else if (byteSize > 1024) {
        return frameStack->formatStr("%.0f KB", round((double)byteSize/1024));
    }
    else {
        return frameStack->formatStr("%.0f bytes", round((double)byteSize));
    }
}

char * MrManager::frameFormatStr(char const * format, ...) {
    assert(frameStack && "Frame stack not initialized.");
    va_list args;
    va_start(args, format);
    char * str = frameStack->vformatStr(format, args);
    va_end(args);
    return str;
}

// -------------------------------------------------------------------------- //
// INPUT/EVENT
// -------------------------------------------------------------------------- //

void MrManager::processInputs() {
    for (int i = 0; i < mm.inputQueue.count; ++i) {
        Input & input = mm.inputQueue.inputs[i];
        switch (input.type) {
        // case Input::Window:
        //     break;

        case Input::Key:
            keyInput(input);
            break;
        case Input::Char:
            charInput(input);
            break;

        case Input::MousePos:
            mousePosInput(input);
            break;
        case Input::MouseButton:
            mouseButtonInput(input);
            break;
        case Input::MouseScroll:
            scrollInput(input);
            break;

        default:
        case Input::None:
            break;
        }
    }
    mm.inputQueue.clear();
}

void MrManager::keyInput(Input const & i) {
    if (i.action == GLFW_PRESS) {
        #if DEV_INTERFACE
        int numKey = glfwNumberKey(i.key);
        if (numKey != -1) {
            setDevState((DevState)numKey);
        }
        #endif // DEV_INTERFACE
    }
    if (setup.keyInput) setup.keyInput(i);
}

void MrManager::charInput(Input const & i) {
    if (setup.charInput) setup.charInput(i);
}

void MrManager::mousePosInput(Input const & i) {
    mousePrevPos = mousePos;
    mousePos.x = i.x;
    mousePos.y = i.y;
    if (setup.cameraControl) cameraControl.mousePosInput(i);
    if (setup.mousePosInput) setup.mousePosInput(i);
}

void MrManager::mouseButtonInput(Input const & i) {
    mouseIsDown = i.action;
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
    if (setup.mouseButtonInput) setup.mouseButtonInput(i);
}

void MrManager::scrollInput(Input const & i) {
    if (setup.cameraControl) cameraControl.scrollInput(i);
    if (setup.scrollInput) setup.scrollInput(i);
}

// -------------------------------------------------------------------------- //
// DEBUG/TESTING
// -------------------------------------------------------------------------- //

#if DEV_INTERFACE

void MrManager::setDevState(DevState value) {
    if (devState == value) {
        return;
    }
    if (value == DEV_STATE_STATS || value == DEV_STATE_TEXT) {
        rendSys.settings.tempDisableMSAA();
    }
    else if (value != DEV_STATE_STATS && value != DEV_STATE_TEXT) {
        rendSys.settings.reenableMSAA();
    }

    devState = value;

    // update bgfx
    // (imgui display set by run loop)
    switch (devState) {
    case DEV_STATE_STATS:
        bgfx::setDebug(BGFX_DEBUG_STATS);
        break;
    case DEV_STATE_TEXT:
        bgfx::setDebug(BGFX_DEBUG_TEXT);
        break;
    case DEV_STATE_WIREFRAME:
        bgfx::setDebug(BGFX_DEBUG_WIREFRAME);
        break;
    default:
        bgfx::setDebug(BGFX_DEBUG_NONE);
    }
}

void MrManager::showDevStateKeyboardShortcuts() {
    char const * summary = "\n"
    "PRESS    FOR DEBUG OVERLAY         \n"
    "0        none                      \n"
    "1        developer interface       \n"
    "2        bgfx stats                \n"
    "3        bgfx debgug text output   \n"
    "4        wireframes                \n"
    "\n";
    print("%s", summary);
}

bool MrManager::isShowingDevInterface() {
    return (devState == DEV_STATE_INTERFACE);
}

#endif // DEV_INTERFACE

void MrManager::test() {
    // char const * absPathA = "/tmp";
    // char const * absPathB = "/Users/Shared";
    // // multiple calls that use mm.frameStack in the same frame
    // char const * relPathA = relPath(absPathA);
    // char const * relPathB = relPath(absPathB);
    // print(
    //     "path a abs: %s\n"
    //     "       rel: %s (%p)\n"
    //     "path b abs: %s\n"
    //     "       rel: %s (%p)\n",
    //     absPathA, relPathA, relPathA,
    //     absPathB, relPathB, relPathB
    // );
}
