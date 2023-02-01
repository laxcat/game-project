#pragma once

// Dear IMGUI implementation using GLFW for input, and BGFX for rendering

#include "../types.h"
#include <bgfx/bgfx.h>
#include <imgui.h>
#include "../glfw.h"
#include "../EventQueue.h"

void imguiCreate(GLFWwindow * window, bgfx::ViewId viewId, ImVec2 windowSize, ImVec2 framebufferSize);
void imguiDestroy();

void imguiBeginFrame(GLFWwindow * window, EventQueue & event, double dt);
void imguiEndFrame(bgfx::ViewId viewId);
