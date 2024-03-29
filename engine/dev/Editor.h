#pragma once
#include <stddef.h>
#include "../common/debug_defines.h"

class Renderable;

class MemEditWindow {
public:
    void * ptr = nullptr;
    size_t size = 0;

    static constexpr size_t TitleSize = 128;
    char title[TitleSize];

    bool open = false;
    float width;
    float height;

    // TODO:
    // void * highlightedPtr;
    // size_t highlightedSize;

    void * winPtr = nullptr;
};

class Editor {
public:

    void init();

    void tick();

    // main sidebar window
    void guiRendering();
    void guiLighting();
    void guiCamera();
    void guiHelpers();
    // void guiRenderables();
    // void guiRenderable(Renderable * r, bool defaultOpen = false);
    void guiGobjs();
    void guiAddGobj();
    void guiFog();
    void guiColors();
    void guiMem();

    #if DEBUG
    void guiDebugger();
    #endif // DEBUG

    // separate movable windows
    void guiWinMem();

    void showMemEditWindow(char const * title, void * ptr, size_t size);
    void clearMemEditWindow();

    MemEditWindow memWin;

};
