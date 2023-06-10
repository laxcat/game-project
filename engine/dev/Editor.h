#pragma once

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
    void guiMem2();

    // separate movable windows
    void guiWinMem();
    void clearMemEditWindow();
};
