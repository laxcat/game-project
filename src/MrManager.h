#pragma once
#include <fstream>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <bgfx/bgfx.h>
#include <entt/entity/registry.hpp>
#include <entt/entity/snapshot.hpp>
#include <cereal/archives/binary.hpp>
#include "animation/Animator.h"
#include "common/glfw_extra.h"
#include "common/InputSys.h"
#include "common/utils.h"
#include "common/MemorySystem.h"
#include "develop/DevOverlay.h"
#include "develop/Editor.h"
#include "develop/print.h"
#include "components/all.h"
#include "render/Camera.h"
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
    // glm::vec3 pos;
    entt::registry registry;
    double thisTime;
    double prevTime;

    MemorySystem memSys;
    RenderSystem rendSys;
    DevOverlay devOverlay;
    Camera camera;
    InputSys input;

    Editor editor;

    Tester tr;

// -------------------------------------------------------------------------- //
// LIFECYCLE
// -------------------------------------------------------------------------- //
    void init(double time) {
        thisTime = time;
        prevTime = time;

        memSys.init();
        rendSys.init();
        devOverlay.init(WindowSize);
        camera.init(WindowSize);

        // loadAllEntities();

        for (int i = 0; i < componentCount; ++i) {
            print("type %s\n", allComponents[i]);
        }

        devOverlay.setState(DevOverlay::DearImGUI);
        devOverlay.showKeyboardShortcuts();

        TestQuadSystem::init();
        // tr.init();
        // rendSys.createFromGLTF("box.glb");
    }

    void shutdown() {
        TestQuadSystem::shutdown();
        memSys.shutdown();
        rendSys.shutdown();
        devOverlay.shutdown();
    }

    void gui() {
        editor.gui();
        // ImGui::ShowDemoWindow();
    }

    void tick() {
        Animator::tick(thisTime);

        bgfx::setViewClear(mainView, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH, rendSys.colors.background);
        // bgfx::setViewTransform(mainView, viewMat, projMat);
        bgfx::setViewTransform(mainView, glm::value_ptr(camera.viewMat), glm::value_ptr(camera.projMat));
        bgfx::setViewRect(mainView, 0, 0, bgfx::BackbufferRatio::Equal);

        devOverlay.tick();

        bgfx::touch(mainView);
        rendSys.draw();
    }


// -------------------------------------------------------------------------- //
// EVENT
// -------------------------------------------------------------------------- //
    void keyEvent(int key, int scancode, int action, int mods) {
        int numKey = glfwNumberKey(key);
        if (numKey != -1 && action == GLFW_PRESS) {
            devOverlay.setState(numKey);
        }
        // // save (cmd+S for mac. ctrl+s for others)
        // if (key == GLFW_KEY_S && action == GLFW_PRESS && mods == GLFW_MOD_SUPER) {
        //     saveAllEntities();
        // }
    }

    void mousePosEvent(double x, double y) {
        input.mousePosEvent(x, y);
    }

    void mouseButtonEvent(int button, int action, int mods) {
        input.mouseButtonEvent(button, action, mods);
    }

    void scrollEvent(double x, double y) {
        input.scrollEvent(x, y);
    }


private:
// -------------------------------------------------------------------------- //
// UTIL
// -------------------------------------------------------------------------- //
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
            print("WARNING: DID NOT LOAD ENTITIES. %s\n", e.what());
            return;
        }
    }
};


inline MrManager mm;