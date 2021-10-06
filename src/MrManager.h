#pragma once
#include <bgfx/bgfx.h>
// #include <glm/vec3.hpp>
// #include <glm/gtx/string_cast.hpp>
// #include <entt/entity/registry.hpp>
#include <imgui.h>
#include "common/utils.h"


struct PosColorVertex {
    float m_x;
    float m_y;
    float m_z;
    uint32_t m_abgr;

    static void init()
    {
        layout
            .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0,   4, bgfx::AttribType::Uint8, true)
            .end();
    };

    static bgfx::VertexLayout layout;
};
bgfx::VertexLayout PosColorVertex::layout;

PosColorVertex verts[] = {
    {0.f, 0.f, 0.f, 0xff0000ff},
    {1.f, 0.f, 0.f, 0x00ff00ff},
    {1.f, 1.f, 0.f, 0x0000ffff},
    {0.f, 1.f, 0.f, 0xff00ffff},
};

uint16_t indices[] = {
    0, 1, 2,
    0, 2, 3,
};


class MrManager {
// -------------------------------------------------------------------------- //
// PUBLIC SETTINGS
// -------------------------------------------------------------------------- //
public:
    size2 const WindowSize = {1280, 720};


// -------------------------------------------------------------------------- //
// STATE VARS
// -------------------------------------------------------------------------- //
    bool showBGFXStats = false;
    // glm::vec3 pos;
    // entt::registry registry;
    double thisTime;
    double prevTime;


// -------------------------------------------------------------------------- //
// LIFECYCLE
// -------------------------------------------------------------------------- //
    void init(double time) {
        updateBGFXDebug();
        thisTime = time;
        prevTime = time;

        PosColorVertex::init();
    }

    void tick() {
        // ImGui::Begin("Fart window");
        // ImGui::Button("Hello!");
        // ImGui::End();
    }


// -------------------------------------------------------------------------- //
// EVENT
// -------------------------------------------------------------------------- //
    void keyEvent(int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
            showBGFXStats = !showBGFXStats;
            updateBGFXDebug();
        }
    }

    void mousePosEvent(double x, double y) {

    }

    void mouseButtonEvent(int button, int action, int mods) {

    }


// -------------------------------------------------------------------------- //
// UTIL
// -------------------------------------------------------------------------- //
private:
    void updateBGFXDebug() {
        bgfx::setDebug(showBGFXStats ? BGFX_DEBUG_STATS : BGFX_DEBUG_NONE);
    }
};