#pragma once
#include <bgfx/bgfx.h>
#include <tiny_gltf.h>
#include <glm/mat4x4.hpp>
#include "Mesh.h"


class Renderable {
public:
    // handled or required by manager
    bool active = false;
    char const * key = "";
    bgfx::ProgramHandle program;

    byte_t * buffer;
    size_t bufferSize;

    // loader or initializer should populate
    std::vector<Mesh> meshes;

    // model
    glm::mat4 model{1.f};

    void draw();
};
