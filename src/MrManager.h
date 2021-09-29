#pragma once
#include <bgfx/bgfx.h>
#include <glm/vec3.hpp>
#include <glm/gtx/string_cast.hpp>
#include <entt/entt.hpp>
#include <imgui.h>
#include "common/utils.h"


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


// -------------------------------------------------------------------------- //
// LIFECYCLE
// -------------------------------------------------------------------------- //
	void init() {
        updateBGFXDebug();
	}

	void tick(double time) {
		auto e = registry.create();
		// printf("wut %p\n", &e);
	}

	void glfwEvent(GLFWwindow *window, int key, int scancode, int action, int mods) {
	    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
	        showBGFXStats = !showBGFXStats;
	        updateBGFXDebug();
	    }
	}


// -------------------------------------------------------------------------- //
// UTIL
// -------------------------------------------------------------------------- //
private:
	void updateBGFXDebug() {
		bgfx::setDebug(showBGFXStats ? BGFX_DEBUG_STATS : BGFX_DEBUG_NONE);
	}
};