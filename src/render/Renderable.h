#pragma once
#include <bgfx/bgfx.h>
#include "PosColorVertex.h"


class Renderable {
public:
    bgfx::DynamicVertexBufferHandle vbh;
    bgfx::IndexBufferHandle ibh;

    PosColorVertex * verts;
    size_t vertCount;
    size_t vertsByteSize;

    virtual void init() {}
    virtual void draw() {}
    virtual void shutdown() {}

    bgfx::Memory const * vertsRef() const {
        return bgfx::makeRef(verts, vertsByteSize);
    };
};
