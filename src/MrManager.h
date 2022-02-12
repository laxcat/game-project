#pragma once
#include <fstream>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <bgfx/bgfx.h>
#include <entt/entity/registry.hpp>
#include <entt/entity/snapshot.hpp>
#include <cereal/archives/binary.hpp>
#include "animation/Animator.h"
#include "common/utils.h"
#include "common/MemorySystem.h"
#include "debug/print.h"
#include "editor/Editor.h"
#include "components/all.h"
#include "render/RenderSystem.h"
#include "render/TestQuadSystem.h"
#include "render/Tester.h"


class MrManager {
public:
// -------------------------------------------------------------------------- //
// PUBLIC SETTINGS
// -------------------------------------------------------------------------- //
    size2 const WindowSize = {1920, 1080};
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

    MemorySystem memSys;
    RenderSystem rendSys;

    float viewMat[16];
    float projMat[16];

    Editor editor;

    Tester tr;

// -------------------------------------------------------------------------- //
// LIFECYCLE
// -------------------------------------------------------------------------- //
    void init(double time) {
        // Set view 0 to the same dimensions as the window and to clear the color buffer

        // bgfx::setDebug(BGFX_DEBUG_PROFILER);
        updateBGFXDebug();
        thisTime = time;
        prevTime = time;

        memSys.init();

        rendSys.init();

        const bx::Vec3 at  = { 0.0f, 0.0f,   0.0f };
        const bx::Vec3 eye = { 0.0f, 2.0f,  -2.0f };

        bx::mtxLookAt(viewMat, eye, at);

        float ratio = float(WindowSize.w)/float(WindowSize.h);
        bx::mtxProj(projMat, 60.0f, ratio, 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);

        loadAllEntities();

        for (int i = 0; i < componentCount; ++i) {
            printf("type %s\n", allComponents[i]);
        }

        TestQuadSystem::init();
        // tr.init();
        // rendSys.createFromGLTF("box.glb");
    }

    void shutdown() {
        TestQuadSystem::shutdown();
        memSys.shutdown();
        rendSys.shutdown();
    }

    void gui() {
        editor.gui();
        // ImGui::ShowDemoWindow();
    }

    void tick() {
        Animator::tick(thisTime);

        bgfx::setViewClear(mainView, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH, rendSys.colors.background);
        bgfx::setViewTransform(mainView, viewMat, projMat);
        bgfx::setViewRect(mainView, 0, 0, bgfx::BackbufferRatio::Equal);
        bgfx::touch(mainView);

        rendSys.draw();
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


private:
// -------------------------------------------------------------------------- //
// UTIL
// -------------------------------------------------------------------------- //
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
            // .component<NewRenderable>(output)
            // .component<NewMesh>(output);
    }

    void loadAllEntities() {
        try {
            std::ifstream storage;
            storage.open(entitiesBinPath);
            cereal::BinaryInputArchive input{storage};
            entt::snapshot_loader{registry}
                .entities(input)
                .component<Info>(input);
                // .component<NewRenderable>(input)
                // .component<NewMesh>(input);
        }
        catch (std::exception const & e) {
            fprintf(stderr, "WARNING: DID NOT LOAD ENTITIES. %s\n", e.what());
            return;
        }
    }
};


inline MrManager mm;