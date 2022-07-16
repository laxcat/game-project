#pragma once

class Editor {
public:
    
    void tick();

    void guiRendering();
    void guiLighting();
    void guiCamera();
    void guiHelpers();
    void guiGLTFs();
    void guiGLTF(char const * label, char const * key);
    void guiFog();
    void guiColors();
};
