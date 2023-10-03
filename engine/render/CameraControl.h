#pragma once
#include <math.h>
#include <glm/vec2.hpp>
#include "../engine.h"

class CameraControl {
public:
    float orthoPanScale = 1.f;
    float orthoScrollScale = 1.f;

    float pitchMax = +M_PI_2 * 0.95f;
    float pitchMin = -M_PI_2 * 0.95f;
    float distMin = 0.01;

    void mousePosEvent(Event const & e);
    void scrollEvent(Event const & e);
};
