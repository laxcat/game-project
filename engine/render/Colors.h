#pragma once
#include <bgfx/bgfx.h>
#include <glm/vec4.hpp>
#include "../common/types.h"
#include "../common/UniColor.h"


// a place to put our colors, accessible by imgui, shaders, etc
class Colors {
public:
    UniColor background     = 0x888888ff;

    void init() {
        background.handle = bgfx::createUniform("u_bgColor", bgfx::UniformType::Vec4);
    }

    void shutdown() {
        bgfx::destroy(background.handle);
    }

};
