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

    memMan.init(setup, &frameStack);
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
    }

    // test();

    return 0;
}

void MrManager::shutdown() {
    if (setup.preShutdown) setup.preShutdown();
    rendSys.shutdown();
    camera.shutdown();
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

char * MrManager::frameStr(size_t size) {
    assert(frameStack && "Frame stack not initialized.");
    char * ret = frameStack->alloc<char>(size);
    assert(ret && "Could not allocate temp string.");
    return ret;
}

char const * MrManager::frameByteSizeStr(size_t byteSize) {
    // " %.0f MB"
    char * ret = mm.frameStr(16);
    if (byteSize > 1024*1024) {
        snprintf(ret, 16, "%.0f MB", round((double)byteSize/(1024*1024)));
    }
    else if (byteSize > 1024) {
        snprintf(ret, 16, "%.0f KB", round((double)byteSize/1024));
    }
    else {
        snprintf(ret, 16, "%.0f bytes", round((double)byteSize));
    }
    return (char const *)ret;
}

char const * MrManager::frameFormatStr(size_t bufferSize, char const * format, ...) {
    char * str = frameStr(bufferSize);
    va_list args;
    va_start(args, format);
    vsnprintf(str, bufferSize, format, args);
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
