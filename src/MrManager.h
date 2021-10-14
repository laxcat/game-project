#pragma once
#include <sstream>
#include <fstream>
#include <GLFW/glfw3.h>
#include <bgfx/bgfx.h>
#include <entt/entity/registry.hpp>
#include <entt/entity/snapshot.hpp>
#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include "common/utils.h"
#include "common/Memory.h"
#include "editor/Editor.h"


struct PosColorVertex {
    float x;
    float y;
    float z;
    uint32_t abgr;

    static void init() {
        layout
            .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0,   4, bgfx::AttribType::Uint8, true)
            .end();
    }

    float * getColor3(float * color) {
        color[0] = ((abgr >>  0) & 0xff) / 255.f; // r
        color[1] = ((abgr >>  8) & 0xff) / 255.f; // g
        color[2] = ((abgr >> 16) & 0xff) / 255.f; // b
        return color;
    }

    void setColor3(float * color) {
        abgr = 
            (((abgr >> 24) & 0xff)      << 24) | // a
            (byte_t(color[2] * 0xff)    << 16) | // b
            (byte_t(color[1] * 0xff)    <<  8) | // g
            (byte_t(color[0] * 0xff)    <<  0);  // r

    }

    static inline bgfx::VertexLayout layout;
};


struct Info {
    char name[16];

    Info() : Info("") {} // entt snapshot seems to require default constructor

    Info(char const * name) {
        strcpy(this->name, name);
    }

    template<class Archive>
    void serialize(Archive & archive) {
        archive(name);
    }
};

class MrManager {
// -------------------------------------------------------------------------- //
// PUBLIC SETTINGS
// -------------------------------------------------------------------------- //
public:
    size2 const WindowSize = {1280, 720};
    static constexpr char const * entitiesBinPath = "entities.bin";

// -------------------------------------------------------------------------- //
// STATE VARS
// -------------------------------------------------------------------------- //
    bgfx::ViewId const mainView = 0;
    bgfx::ViewId const guiView  = 0xff;
    bool showBGFXStats = false;
    bool showImGUI = true;
    // glm::vec3 pos;
    entt::registry registry;
    double thisTime;
    double prevTime;

    bgfx::ProgramHandle program;

    Memory mem;
    float viewMat[16];
    float projMat[16];

    Editor editor;

    static size_t const vertCount = 4;
    PosColorVertex verts[vertCount] = {
        {0.f, 0.f, 0.f, 0xffff0000},
        {1.f, 0.f, 0.f, 0xff00ff00},
        {1.f, 1.f, 0.f, 0xff0000ff},
        {0.f, 1.f, 0.f, 0xffff00ff},
    };
    float vertFloatColors[vertCount*4];
    bgfx::DynamicVertexBufferHandle vbh;
    bgfx::Memory const * vbRef;

    uint16_t indices[6] = {
        0, 1, 2,
        0, 2, 3,
    };
    bgfx::IndexBufferHandle ibh;



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

        vbRef = bgfx::makeRef(verts, sizeof(verts));
        vbh = bgfx::createDynamicVertexBuffer(vbRef, PosColorVertex::layout);

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

        loadAllEntities();
    }

    void shutdown() {
        mem.shutdown();
    }

    void gui() {
        editor.gui();
        // ImGui::ShowDemoWindow();
    }

    void tick() {

        bgfx::setViewTransform(0, viewMat, projMat);
        bgfx::setViewRect(mainView, 0, 0, bgfx::BackbufferRatio::Equal);

        bgfx::touch(mainView);
        
        bgfx::update(vbh, 0, bgfx::makeRef(verts, sizeof(verts)));
        bgfx::setVertexBuffer(mainView, vbh);
        bgfx::setIndexBuffer(ibh);
        bgfx::setState(BGFX_STATE_WRITE_RGB);
        
        bgfx::submit(mainView, program);
    }


// -------------------------------------------------------------------------- //
// EVENT
// -------------------------------------------------------------------------- //
    void keyEvent(int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
            showBGFXStats = !showBGFXStats;
            updateBGFXDebug();
        }
        if (key == GLFW_KEY_GRAVE_ACCENT && action == GLFW_PRESS) {
            showImGUI = !showImGUI;
            // auto old = debugState;
            // debugState = DebugState(uint8_t(debugState) + 1);
            // if (debugState == DebugState::_Count) debugState = DebugState::Off;
            // updateBGFXDebug();
        }
        // save (cmd+S for mac. ctrl+s for others)
        if (key == GLFW_KEY_S && action == GLFW_PRESS && mods == GLFW_MOD_SUPER) {
            saveAllEntities();
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

    void saveAllEntities() {
        // std::stringstream storage;
        std::ofstream storage;
        storage.open(entitiesBinPath);
        cereal::BinaryOutputArchive output{storage};
        entt::snapshot{registry}
            .entities(output)
            .component<Info>(output);
    }

    void loadAllEntities() {
        std::ifstream storage;
        storage.open(entitiesBinPath);
        cereal::BinaryInputArchive input{storage};
        entt::snapshot_loader{registry}
            .entities(input)
            .component<Info>(input);
    }
};


inline MrManager mm;