#pragma once
#include "../shader/shaders/gltf/VertGLTF.h"


namespace Quad {

    inline Mesh & create(
            Renderable * r, 
            VertGLTF * vbuffer, 
            uint16_t * ibuffer, 
            glm::vec2 size,
            int materialId = -1,
            bool dynamic = false
        ) {
        float w2 = size.x/2;
        float h2 = size.y/2;

        // y-up
        VertGLTF vdata[] = {
            // pos           // norm            // tex
            {-w2, -h2, 0,    0.f, 0.f, 1.f,     0.f, 0.f},
            {+w2, -h2, 0,    0.f, 0.f, 1.f,     1.f, 0.f},
            {+w2, +h2, 0,    0.f, 0.f, 1.f,     1.f, 1.f},
            {-w2, +h2, 0,    0.f, 0.f, 1.f,     0.f, 1.f},
        };

        // cw winding
        uint16_t idata[] = {
            0, 2, 1, 
            0, 3, 2,
        };

        Mesh m;
        m.renderableKey = r->key;
        m.materialId = materialId;

        memcpy(ibuffer, idata, sizeof(idata));
        auto iref = bgfx::makeRef(ibuffer, sizeof(idata));
        m.ibuf = bgfx::createIndexBuffer(iref);

        // TODO: move layout to more central location
        bgfx::VertexLayout layout;
        layout.begin();
        layout.add(bgfx::Attrib::Position,  3, bgfx::AttribType::Float);
        layout.add(bgfx::Attrib::Normal,    3, bgfx::AttribType::Float);
        layout.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float);
        layout.end();

        memcpy(vbuffer, vdata, sizeof(vdata));
        auto vref = bgfx::makeRef(vbuffer, sizeof(vdata));
        if (dynamic) {
            m.dynvbuf = bgfx::createDynamicVertexBuffer(vref, layout);
        }
        else {
            auto vbuf = bgfx::createVertexBuffer(vref, layout);
            m.vbufs.push_back(vbuf);
        }

        r->meshes.push_back(m);

        return r->meshes.back();
    }

    inline size_t vbufferSize() {
        return sizeof(VertGLTF) * 4;
    }

    inline size_t ibufferSize() {
        return sizeof(uint16_t) * 6;
    }

    inline size_t buffersSize(size_t count = 1) {
        return (vbufferSize() + ibufferSize()) * count;
    }

    inline void allocateBufferWithCount(Renderable * r, size_t count = 1) {
        r->bufferSize = buffersSize(count);
        r->buffer = (byte_t *)malloc(r->bufferSize);
    }

    // !!!
    // buffer layout: 
    // quad 0 vbuf, quad 0 ibuf, quad 1 vbuf, quad 1 ibuf, etc
    inline VertGLTF * vbufferForIndex(Renderable * r, size_t index) {
        return (VertGLTF *)(r->buffer + buffersSize(index));
    }
    inline uint16_t * ibufferForIndex(Renderable * r, size_t index) {
        return (uint16_t *)(r->buffer + buffersSize(index) + vbufferSize());
    }

    inline Mesh & create(
            Renderable * r, 
            size_t index,
            glm::vec2 size,
            int materialId = -1,
            bool dynamic = false
        ) {
        return create(r, vbufferForIndex(r, index), ibufferForIndex(r, index), size, materialId, dynamic);
    }
}
