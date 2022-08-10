#pragma once
#include <glm/vec2.hpp>
#include "../engine.h"

class CameraControl {
public:
    float orthoPanScale = 1.f;
    float orthoScrollScale = 1.f;

    void mousePosEvent(Event const & e);
    void scrollEvent(Event const & e);
};
