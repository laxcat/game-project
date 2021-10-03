#pragma once
#include <bgfx/bgfx.h>
#include <glm/vec3.hpp>
#include <glm/gtx/string_cast.hpp>
#include <entt/entt.hpp>
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


// -------------------------------------------------------------------------- //
// LIFECYCLE
// -------------------------------------------------------------------------- //
	void init() {
        updateBGFXDebug();
        IMGUI_CHECKVERSION();
    	ImGui::CreateContext();
    	ImGui_Implbgfx_Init(0);
    	ImGui::GetIO().DisplaySize = {(float)WindowSize.w, (float)WindowSize.h};
	}

	void tick(double time) {
		auto e = registry.create();
		// printf("wut %p\n", &e);

		ImGui_Implbgfx_NewFrame();
		ImGui::NewFrame();

        ImGui::Begin("Fart window");
        ImGui::Button("Hello!");
        ImGui::End();
        ImGui::Render();
        ImGui_Implbgfx_RenderDrawLists(ImGui::GetDrawData());
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