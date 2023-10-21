#pragma once

// Dear IMGUI implementation using GLFW for input, and BGFX for rendering

#include "../types.h"
#include <bgfx/bgfx.h>
#include <imgui.h>
#include "../glfw.h"
#include "../InputQueue.h"

void imguiCreate(GLFWwindow * window, bgfx::ViewId viewId, ImVec2 windowSize);
void imguiDestroy();
void imguiBeginFrame(size2 windowSize, InputQueue & queue, double dt);
void imguiEndFrame();
