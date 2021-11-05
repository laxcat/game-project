#pragma once
#include <vector>
#include "../common/types.h"


class Mesh {
public:

    std::vector<bgfx::VertexBufferHandle> vbufs;
    std::vector<bgfx::DynamicVertexBufferHandle> dvbufs;
    bgfx::IndexBufferHandle ibuf;

    byte_t * verts;
    size_t vertsTotalByteSize;
    size_t vertCount;
    size_t vertByteSize;
    
    // Utility
    bgfx::Memory const * vertsRef() const {
        return bgfx::makeRef(verts, vertsTotalByteSize);
    };

    void initVerts(size_t vertCount_, size_t vertByteSize_) {
        freeVerts();
        vertCount = vertCount_;
        vertByteSize = vertByteSize_;
        vertsTotalByteSize = vertCount * vertByteSize_;
        verts = (byte_t *)malloc(vertsTotalByteSize);
    }

    void freeVerts() {
        if (verts) return;
        free(verts);
        verts = nullptr;
    }

    template <typename T>
    T * vertAt(size_t index) {
        return ((T *)verts) + index;
    }
};
