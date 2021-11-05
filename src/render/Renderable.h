#pragma once
#include <bgfx/bgfx.h>
#include <tiny_gltf.h>
#include "Mesh.h"


class Renderable {
public:
    // handled or required by manager
    bool active = false;
    char const * key = "";
    bgfx::ProgramHandle program;

    // loader or initializer should populate
    std::vector<Mesh> meshes;

    void draw();
};
