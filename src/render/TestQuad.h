#pragma once
#include "Renderable.h"


class TestQuad {
public:
    size_t renderable = -1;
    
    void init();
    void shutdown();

    float * getColor3(int index, float * color);
    void setColor3(int index, float * color);
};
