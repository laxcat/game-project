#pragma once
#include <bgfx/bgfx.h>
#include <imgui.h>
#include "common/utils.h"
#include "common/Memory.h"


struct PosColorVertex {
    float x;
    float y;
    float z;
    uint32_t abgr;

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
    {0.f, 0.f, 0.f, 0xffff0000},
    {1.f, 0.f, 0.f, 0xff00ff00},
    {1.f, 1.f, 0.f, 0xff0000ff},
    {0.f, 1.f, 0.f, 0xffff00ff},
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
    bgfx::ViewId const mainView = 0;
    bool showBGFXStats = false;
    // glm::vec3 pos;
    // entt::registry registry;
    double thisTime;
    double prevTime;

    bgfx::VertexBufferHandle vbh;
    bgfx::IndexBufferHandle ibh;
    bgfx::ProgramHandle program;

    Memory mem;

    float viewMat[16];
    float projMat[16];


// -------------------------------------------------------------------------- //
// LIFECYCLE
// -------------------------------------------------------------------------- //
    void init(double time) {
        // Set view 0 to the same dimensions as the window and to clear the color buffer
        bgfx::setViewClear(mainView, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH);

        updateBGFXDebug();
        thisTime = time;
        prevTime = time;

        mem.init();
        PosColorVertex::init();

        vbh = bgfx::createVertexBuffer(bgfx::makeRef(verts, sizeof(verts)), PosColorVertex::layout);
        ibh = bgfx::createIndexBuffer(bgfx::makeRef(indices, sizeof(indices)));
        program = mem.loadProgram("vs_main", "fs_main");

        const bx::Vec3 at  = { 0.5f, 0.5f,   0.0f };
        const bx::Vec3 eye = { 0.5f, 0.5f,  -2.0f };

        bx::mtxLookAt(viewMat, eye, at);

        float ratio = float(WindowSize.w)/float(WindowSize.h);
        bx::mtxProj(projMat, 60.0f, ratio, 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);

        for (int i = 0; i < 4; ++i) {
            printf("vert %d: %08x\n", i, verts[i].abgr);
        }
    }

    void shutdown() {
        mem.shutdown();
    }

    void tick() {
        // ImGui::Begin("Fart window");
        // ImGui::Button("Hello!");
        // ImGui::End();

        bgfx::setViewTransform(0, viewMat, projMat);
        bgfx::setViewRect(mainView, 0, 0, bgfx::BackbufferRatio::Equal);
        bgfx::touch(mainView);
        bgfx::setVertexBuffer(mainView, vbh);
        bgfx::setIndexBuffer(ibh);
        bgfx::setState(BGFX_STATE_WRITE_RGB);
        bgfx::submit(mainView, program);
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