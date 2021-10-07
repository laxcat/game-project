#pragma once
#include <bgfx/bgfx.h>
#include <imgui.h>
#include "common/utils.h"
#include "common/Memory.h"


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
    bool showBGFXStats = true;
    // glm::vec3 pos;
    // entt::registry registry;
    double thisTime;
    double prevTime;

    bgfx::VertexBufferHandle vbh;
    bgfx::IndexBufferHandle ibh;
    bgfx::ProgramHandle program;

    Memory mem;


// -------------------------------------------------------------------------- //
// LIFECYCLE
// -------------------------------------------------------------------------- //
    void init(double time) {
        updateBGFXDebug();
        thisTime = time;
        prevTime = time;

        mem.init();
        PosColorVertex::init();

        vbh = bgfx::createVertexBuffer(bgfx::makeRef(verts, sizeof(verts)), PosColorVertex::layout);
        ibh = bgfx::createIndexBuffer(bgfx::makeRef(indices, sizeof(indices)));
        program = mem.loadProgram("vs_main", "fs_main");
    }

    void shutdown() {
        mem.shutdown();
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
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
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