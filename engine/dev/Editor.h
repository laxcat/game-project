#pragma once
#include <stddef.h>

class MemEditWindow {
public:
    void * ptr = nullptr;
    size_t size = 0;

    static constexpr size_t TitleSize = 128;
    char title[TitleSize];

    // TODO:
    // void * highlightedPtr;
    // size_t highlightedSize;

    void * winPtr = nullptr;
};

class Editor {
public:
    Editor();

    void tick();

    // main sidebar window
    void guiRendering();
    void guiLighting();
    void guiCamera();
    void guiHelpers();
    void guiGLTFs();
    void guiGLTF(char const * label, char const * key);
    void guiFog();
    void guiColors();
    void guiMem();

    // separate movable windows
    void guiWinMem();

    void showMemEditWindow(void * ptr, size_t size);
    void clearMemEditWindow();

    MemEditWindow memWin;
};
