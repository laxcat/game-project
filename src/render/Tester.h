#pragma once
#include "Renderable.h"


class Tester {
public:
    size_t renderable = -1;
    
    void init();

    float * getColor3(int index, float * color);
    void setColor3(int index, float * color);
};
