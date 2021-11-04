#pragma once
#include <vector>


class Mesh {
public:

    std::vector<bgfx::VertexBufferHandle> vbufs;
    std::vector<bgfx::DynamicVertexBufferHandle> dvbufs;
    bgfx::IndexBufferHandle ibuf;

    PosColorVertex * verts;
    size_t vertCount;
    size_t vertsByteSize;
    
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
