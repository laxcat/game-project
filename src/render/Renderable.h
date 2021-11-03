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

    // override these for Renderable subtypes
    virtual void init() {}
    virtual void draw() {}
    virtual void shutdown() {}

    // Utility
    bgfx::Memory const * vertsRef() const {
        return bgfx::makeRef(verts, vertsByteSize);
    };

    void initVerts(size_t vertCount_) {
        freeVerts();
        vertCount = vertCount_;
        vertsByteSize = vertCount * sizeof(PosColorVertex);
        verts = (PosColorVertex *)malloc(vertsByteSize);
    }

    void freeVerts() {
        if (verts) return;
        free(verts);
        verts = nullptr;
    }
};
