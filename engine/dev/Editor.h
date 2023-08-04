#pragma once
#include <stddef.h>
#include "../memory/File.h"
#include "../memory/Array.h"

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

    struct GLTFSlot {
        char label[64];
        char key[64];
        File::Path path;
        GLTFSlot(char const * label, char const * key) {
            snprintf(this->label, 64, "%s", label);
            snprintf(this->key, 64, "%s", key);
        }
    };

    void init();

    void tick();

    // main sidebar window
    void guiRendering();
    void guiLighting();
    void guiCamera();
    void guiHelpers();
    void guiGLTFs();
    void guiGLTF(GLTFSlot & slot);
    void guiFog();
    void guiColors();
    void guiMem();
    void guiDebugger();

    // separate movable windows
    void guiWinMem();

    void showMemEditWindow(char const * title, void * ptr, size_t size);
    void clearMemEditWindow();

    MemEditWindow memWin;

    Array<GLTFSlot> * gltfSlots;
};
