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

    memMan.init(setup);
    if (memMan.firstBlock()->type() == MEM_BLOCK_STACK) {
        frameStack = memMan.getBlockDataAt<Stack>(0);
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

    if (setup.args.c > 1) {
        // memMan.createEntity(setup.args.v[1], true);
    }

    // test();

    return 0;
}

void MrManager::shutdown() {
    if (setup.preShutdown) setup.preShutdown();
    rendSys.shutdown();
    camera.shutdown();
    // memMan.shutdown(); // manually call this later
    if (setup.postShutdown) setup.postShutdown();
}

void MrManager::beginFrame(double nowInSeconds) {
    // update frame
    ++frame;
    memMan.frame = frame;
    // update time
    prevTime = thisTime;
    thisTime = nowInSeconds;
    dt = thisTime - prevTime;
}

void MrManager::endFrame() {
    if (frameStack) frameStack->reset();
}

void MrManager::tick() {
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
    int bufSize = 4096;
    char * buf = tempStr(bufSize);
    memMan.getInfo(buf, bufSize);
    print("%s\n", buf);

    printl("!!! creating 2 pools");
    printl();
    Pool * temp1 = memMan.createPool(1024, 16);
    assert(temp1);
    Pool * temp2 = memMan.createPool(16, 16);
    assert(temp2);

    memMan.getInfo(buf, bufSize);
    print("%s\n", buf);

    printl("!!! destroying pool 1");
    printl();
    memMan.destroyPool(temp1);

    memMan.getInfo(buf, bufSize);
    print("%s\n", buf);

    printl("!!! destroying pool 2");
    printl();
    memMan.destroyPool(temp2);

    memMan.getInfo(buf, bufSize);
    print("%s\n", buf);

    printl("!!! testing frameStack->formatStr");
    printl();
    char * temp = frameStack->formatStr("hello this is %d\n", 47);
    print("%s", temp);

    printl("!!! testing external allocation");
    printl();
    Path * p = memMan.create<Path>(0, "fart");
    assert(p);

    memMan.getInfo(buf, bufSize);
    print("%s\n", buf);

    printl("!!! creating 1 pool");
    printl();
    Pool * temp3 = memMan.createPool(64, 64);
    assert(temp3);

    memMan.getInfo(buf, bufSize);
    print("%s\n", buf);

    printl("!!! destroying external allocation");
    printl();
    memMan.destroy(p);

    memMan.getInfo(buf, bufSize);
    print("%s\n", buf);

    printl("!!! destroying pool 3");
    printl();
    memMan.destroyPool(temp3);

    memMan.getInfo(buf, bufSize);
    print("%s\n", buf);

    printl("loading file");
    printl();
    File * f = memMan.createFileHandle("BoxTextured.glb", true);
    assert(f);
    if (f->loaded()) {
        printl("file loaded. found %zu bytes. last byte: 0x%02x", f->fileSize(), f->data()[f->size()-1]);
    }
    else {
        printl("file read but not loaded. found %zu bytes.", f->size());
    }

    memMan.getInfo(buf, bufSize);
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
