#pragma once
#include <glm/vec2.hpp>


class InputSys {
public:
    bool isDown = false;
    glm::vec2 pos;
    glm::vec2 prevPos;
    glm::vec2 downPos;
    glm::vec2 upPos;

    void mousePosEvent(double x, double y);
    void mouseButtonEvent(int button, int action, int mods);
    void scrollEvent(double x, double y);
};
