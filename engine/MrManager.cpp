#include "MrManager.h"
#include "common/glfw.h"
#include "dev/OriginWidget.h"


// TESTING AND DEBUG ONLY, REMOVE THESE
#include "common/Path.h"
#include "common/string_utils.h"


MrManager mm;

int MrManager::init(EngineSetup const & setup) {
    assert(window && "set window");

    this->setup = setup;

    thisTime = setup.startTime;
    prevTime = setup.startTime;

    int err = 0;
    if (setup.preInit) err = setup.preInit(setup.args);
    if (err) return err;

    memSys.init(setup.memSysSize);
    if (setup.frameStackSize) {
        frameStack = memSys.createStack(setup.frameStackSize);
    }
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

    if (setup.postInit) err = setup.postInit(setup.args);
    if (err) return err;

    test();

    return 0;
}

void MrManager::shutdown() {
    if (setup.preShutdown) setup.preShutdown();
    rendSys.shutdown();
    camera.shutdown();
    memSys.shutdown();
    if (setup.postShutdown) setup.postShutdown();
}

void MrManager::updateTime(double nowInSeconds) {
    prevTime = thisTime;
    thisTime = nowInSeconds;
    dt = thisTime - prevTime;
}

void MrManager::tick() {
    if (setup.preDraw) setup.preDraw();
    draw();
    if (setup.postDraw) setup.postDraw();
    if (frameStack) frameStack->reset();
}

void MrManager::draw() {
    bgfx::setViewClear(mainView, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH, rendSys.colors.background);
    bgfx::setViewTransform(mainView, (float *)&camera.viewMat, (float *)&camera.projMat);
    bgfx::setViewRect(mainView, 0, 0, bgfx::BackbufferRatio::Equal);
    rendSys.draw();
    bgfx::frame();
}

void MrManager::updateSize(size2 windowSize) {
    if (setup.preResize) setup.preResize();

    this->windowSize = windowSize;
    rendSys.settings.updateSize(windowSize);
    camera.setRatio(windowSize);

    if (setup.postResize) setup.postResize();
}

// -------------------------------------------------------------------------- //
// TEMP STRING
// -------------------------------------------------------------------------- //

char * MrManager::tempStr(size_t size) {
    assert(frameStack && "Frame stack not initialized.");
    char * ret = frameStack->alloc<char>(size);
    assert(ret && "Could not allocate temp string.");
    return ret;
}


// -------------------------------------------------------------------------- //
// EVENT
// -------------------------------------------------------------------------- //

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
    int bufSize = 4096;
    char * buf = tempStr(bufSize);
    memSys.getInfo(buf, bufSize);
    print("%s\n", buf);

    printl("!!! creating 2 pools");
    printl();
    MemSys::Pool * temp1 = memSys.createPool(1024, 16);
    MemSys::Pool * temp2 = memSys.createPool(16, 16);

    memSys.getInfo(buf, bufSize);
    print("%s\n", buf);

    printl("!!! destroying pool 1");
    printl();
    memSys.destroyPool(temp1);

    memSys.getInfo(buf, bufSize);
    print("%s\n", buf);

    printl("!!! destroying pool 2");
    printl();
    memSys.destroyPool(temp2);

    memSys.getInfo(buf, bufSize);
    print("%s\n", buf);

    printl("!!! testing frameStack->formatStr");
    printl();
    char * temp = frameStack->formatStr("hello this is %d\n", 47);
    print("%s", temp);

    printl("!!! testing external allocation");
    printl();
    Path * p = memSys.create<Path>(0, "fart");

    memSys.getInfo(buf, bufSize);
    print("%s\n", buf);

    printl("!!! creating 1 pool");
    printl();
    MemSys::Pool * temp3 = memSys.createPool(64, 64);

    memSys.getInfo(buf, bufSize);
    print("%s\n", buf);

    printl("!!! destroying external allocation");
    printl();
    memSys.destroy(p);

    memSys.getInfo(buf, bufSize);
    print("%s\n", buf);

    printl("!!! destroying pool 3");
    printl();
    memSys.destroyPool(temp3);

    memSys.getInfo(buf, bufSize);
    print("%s\n", buf);

    printl("loading file");
    printl();
    MemSys::File * f = memSys.createFileHandle("BoxTextured.glb", true);
    if (f) {
        if (f->loaded()) {
            printl("file loaded. found %zu bytes. last byte: 0x%02x", f->fileSize(), f->data()[f->size()-1]);
        }
        else {
            printl("file read but not loaded. found %zu bytes.", f->size());
        }
    }

    memSys.getInfo(buf, bufSize);
    print("%s\n", buf);

    char const * absPathA = "/tmp";
    char const * absPathB = "/Users/Shared";
    // multiple calls that use mm.frameStack in the same frame
    char const * relPathA = relPath(absPathA);
    char const * relPathB = relPath(absPathB);
    print(
        "path a abs: %s\n"
        "       rel: %s (%p)\n"
        "path b abs: %s\n"
        "       rel: %s (%p)\n",
        absPathA, relPathA, relPathA,
        absPathB, relPathB, relPathB
    );
}
