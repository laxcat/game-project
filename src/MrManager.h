#pragma once
#include <bgfx/bgfx.h>
#include <glm/vec3.hpp>
#include <glm/gtx/string_cast.hpp>
#include <entt/entity/registry.hpp>
#include <imgui.h>
#include "common/utils.h"
#include "bgfx-imgui/imgui_impl_bgfx.h"


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
    glm::vec3 pos;
    entt::registry registry;
    double thisTime;
    double prevTime;


// -------------------------------------------------------------------------- //
// LIFECYCLE
// -------------------------------------------------------------------------- //
    void init(double time) {
        updateBGFXDebug();
        thisTime = time;
        prevTime = time;

        auto a = registry.create();
        auto b = registry.create();
        auto c = registry.create();
        printf("wut %p %zu, %p %zu, %p %zu\n", 
            &a, (size_t)a,
            &b, (size_t)b,
            &c, (size_t)c
        );
    }

    void tick() {
        ImGui::Begin("Fart window");
        ImGui::Button("Hello!");
        ImGui::End();
    }

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