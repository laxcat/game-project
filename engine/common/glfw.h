#pragma once

#define GLFW_INCLUDE_NONE           // prevents including opengl header
#include <GLFW/glfw3.h>


// returns 0-9 for standard or numpad key, -1 if not a number
inline int glfwNumberKey(int key) {
    if (key >= GLFW_KEY_0    && key <= GLFW_KEY_9)    return key - GLFW_KEY_0;
    if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_9) return key - GLFW_KEY_KP_0;
    return -1;
}
