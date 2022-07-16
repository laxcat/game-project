#pragma once
#include <bgfx/bgfx.h>


class Samplers {
public:
    bgfx::UniformHandle color;

    void init() {
        color = bgfx::createUniform("s_texColor",  bgfx::UniformType::Sampler);
    }

    void shutdown() {
        bgfx::destroy(color);
    }
};
