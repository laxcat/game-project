#pragma once
#include "../render/Renderable.h"


class OriginWidget {
public:
    static Renderable * create();
    static void setScale(float s);
    static void updateModelFromScale();
    static float scale;
};
