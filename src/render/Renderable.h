#pragma once
#include <bgfx/bgfx.h>
#include <tiny_gltf.h>
#include "PosColorVertex.h"


class Renderable {
public:
    bool active = false;
    char const * key = "";

    bgfx::DynamicVertexBufferHandle vbh;
    bgfx::IndexBufferHandle ibh;

    PosColorVertex * verts;
    size_t vertCount;
    size_t vertsByteSize;

    virtual void init() {}
    virtual void draw() {}
    virtual void shutdown() {}

    void loadGLTF(char const * filename);
    void traverseGLTFNodes(tinygltf::Model const & model, std::vector<int> const & nodes, int level);

    bgfx::Memory const * vertsRef() const {
        return bgfx::makeRef(verts, vertsByteSize);
    };
};
