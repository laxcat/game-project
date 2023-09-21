#pragma once
#include <bgfx/bgfx.h>
#include <glm/vec4.hpp>


// simple distance clipping with color fade falloff
class Fog {
public:
    bgfx::UniformHandle handle;
    glm::vec4 data;
    float & minDistance = data[0];
    float & fadeDistance = data[1];
    float & amount = data[2];
    // float & unused = data[3] // unused

    void init() {
        handle = bgfx::createUniform("u_fog", bgfx::UniformType::Vec4);
        minDistance = 40.0;
        fadeDistance = 60.0;
        amount = 1.0;
    }

    void shutdown() {
        bgfx::destroy(handle);
    }
};
