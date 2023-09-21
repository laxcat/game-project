#include "MrManager.h"
#include "common/glfw.h"

MrManager mm;

int MrManager::init(EngineSetup const & setup) {
    assert(window && "set window");

    this->setup = setup;

    thisTime = setup.startTime;
    prevTime = setup.startTime;

    memMan.init(setup, &frameStack);
    rendSys.init();
    camera.init(windowSize);
    editor.init();

    workers = memMan.createArray<Worker *>(64);
    workerGroups = memMan.createArray<WorkerGroup>(16);

    #if DEV_INTERFACE
    devOverlay.init(windowSize);
    devOverlay.setState(DevOverlay::DearImGUI);
    devOverlay.showKeyboardShortcuts();
    #endif // DEV_INTERFACE

    // #if DEV_INTERFACE
    // originWidget = OriginWidget::create();
    // originWidget->instances[0].active = false;
    // OriginWidget::setScale(5.f);
    // #endif // DEV_INTERFACE

    if (setup.args.c > 1) {
    }

    // test();

    createWorker([this]{
        // printl("fart");
        // Gobj * g = memMan.createGobj("../../../gltf_assets/CesiumMilkTruck.glb");
        // Gobj * g = memMan.createGobj("../../../gltf_assets/Cameras.gltf");
        // Gobj * g = memMan.createGobj("../../../gltf_assets/CesiumMilkTruck.glb");
        // Gobj * g = memMan.createGobj("../../../gltf_assets/CesiumMilkTruck/CesiumMilkTruck.gltf");
        Gobj * g = memMan.createGobj("../../../gltf_assets/LS500.glb");
        // Gobj * g = memMan.createGobj("../../glTF-Sample-Models/2.0/BoomBox/glTF-Binary/BoomBox.glb");
        // Gobj * g = memMan.createGobj("../../glTF-Sample-Models/2.0/Box/glTF-Binary/Box.glb");
        // Gobj * g = memMan.createGobj("../../glTF-Sample-Models/2.0/BoxInterleaved/glTF-Binary/BoxInterleaved.glb");
        // Gobj * g = memMan.createGobj("../../glTF-Sample-Models/2.0/BoxTextured/glTF-Binary/BoxTextured.glb");
        // Gobj * g = memMan.createGobj("../../glTF-Sample-Models/2.0/ClearCoatCarPaint/glTF-Binary/ClearCoatCarPaint.glb");
        // Gobj * g = memMan.createGobj("../../glTF-Sample-Models/2.0/MaterialsVariantsShoe/glTF-Binary/MaterialsVariantsShoe.glb");
        // Gobj * g = memMan.createGobj("../../glTF-Sample-Models/2.0/MetalRoughSpheres/glTF-Binary/MetalRoughSpheres.glb");
        // Gobj * g = memMan.createGobj("../../glTF-Sample-Models/2.0/MorphStressTest/glTF-Binary/MorphStressTest.glb");
        // Gobj * g = memMan.createGobj("../../glTF-Sample-Models/2.0/MultipleScenes/glTF-Embedded/MultipleScenes.gltf");
        // Gobj * g = memMan.createGobj("../../glTF-Sample-Models/2.0/TextureCoordinateTest/glTF-Binary/TextureCoordinateTest.glb");
        // Gobj * g = memMan.createGobj("../../glTF-Sample-Models/2.0/TwoSidedPlane/glTF/TwoSidedPlane.gltf");
        assert(g && "init gobj failed");
        rendSys.add("test", g);
        // rendSys.remove("test");
    });

    return 0;
}

void MrManager::shutdown() {
    if (setup.preShutdown) setup.preShutdown();
    rendSys.shutdown();
    camera.shutdown();
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
    bgfx::setViewTransform(mainView, (float *)&camera.viewMat, (float *)&camera.projMat);
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
    camera.setRatio(windowSize);

    if (setup.postResize) setup.postResize();
}

// -------------------------------------------------------------------------- //
// WORKERS
// -------------------------------------------------------------------------- //
    Worker * MrManager::createWorker(Worker::Fn const & task, void * group) {
        Worker * w = memMan.create<Worker>(task, group);
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
// EVENT
// -------------------------------------------------------------------------- //

void MrManager::processEvents() {
    for (int i = 0; i < mm.eventQueue.count; ++i) {
        Event & e = mm.eventQueue.events[i];
        switch (e.type) {
        // case Event::Window:
        //     break;

        case Event::Key:
            keyEvent(e);
            break;
        case Event::Char:
            charEvent(e);
            break;

        case Event::MousePos:
            mousePosEvent(e);
            break;
        case Event::MouseButton:
            mouseButtonEvent(e);
            break;
        case Event::MouseScroll:
            scrollEvent(e);
            break;

        default:
        case Event::None:
            break;
        }
    }
    mm.eventQueue.clear();
}

void MrManager::keyEvent(Event const & e) {
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

void MrManager::charEvent(Event const & e) {
    if (setup.charEvent) setup.charEvent(e);
}

void MrManager::mousePosEvent(Event const & e) {
    mousePrevPos = mousePos;
    mousePos.x = e.x;
    mousePos.y = e.y;
    if (setup.cameraControl) cameraControl.mousePosEvent(e);
    if (setup.mousePosEvent) setup.mousePosEvent(e);
}

void MrManager::mouseButtonEvent(Event const & e) {
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

void MrManager::scrollEvent(Event const & e) {
    if (setup.cameraControl) cameraControl.scrollEvent(e);
    if (setup.scrollEvent) setup.scrollEvent(e);
}


// -------------------------------------------------------------------------- //
// TESTING
// -------------------------------------------------------------------------- //

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