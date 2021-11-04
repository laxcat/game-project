#pragma once
#include <bgfx/bgfx.h>
#include <tiny_gltf.h>
#include "PosColorVertex.h"
#include "Mesh.h"


class Renderable {
public:
    bool active = false;
    char const * key = "";

    std::vector<Mesh> meshes;
    bgfx::ProgramHandle program;

    // override these for Renderable subtypes
    virtual void init() {}
    virtual void draw();
    virtual void shutdown() {}
};
