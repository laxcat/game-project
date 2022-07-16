#pragma once
#include "../render/Renderable.h"
#include "../shader/shaders/unlit/VertUnlit.h"


namespace Cube {

    inline Mesh & create(
        Renderable * r, 
        VertUnlit * vbuffer, 
        uint16_t * ibuffer, 
        glm::vec3 const & size, 
        glm::vec3 const & pos,
        std::vector<uint32_t> const & colors = {},
        int materialId = -1
    ) {
        auto hs = size * .5f;
        // z-up // TODO: double check this. should be fine, but it should be y-up
        // Pretty sure this is wrong. front, right, etc, don't match colors. need to be converted to y-up.
        VertUnlit vdata[] = {
            // front (blue)
            {-hs.x, -hs.y, -hs.z, 0xffff0000},
            {+hs.x, -hs.y, -hs.z, 0xffff0000},
            {+hs.x, -hs.y, +hs.z, 0xffff0000},
            {-hs.x, -hs.y, +hs.z, 0xffff0000},
            // right (red)
            {+hs.x, -hs.y, -hs.z, 0xff0000ff},
            {+hs.x, +hs.y, -hs.z, 0xff0000ff},
            {+hs.x, +hs.y, +hs.z, 0xff0000ff},
            {+hs.x, -hs.y, +hs.z, 0xff0000ff},
            // back (green)
            {+hs.x, +hs.y, -hs.z, 0xff00ff00},
            {-hs.x, +hs.y, -hs.z, 0xff00ff00},
            {-hs.x, +hs.y, +hs.z, 0xff00ff00},
            {+hs.x, +hs.y, +hs.z, 0xff00ff00},
            // left (orange)
            {-hs.x, +hs.y, -hs.z, 0xff2288dd},
            {-hs.x, -hs.y, -hs.z, 0xff2288dd},
            {-hs.x, -hs.y, +hs.z, 0xff2288dd},
            {-hs.x, +hs.y, +hs.z, 0xff2288dd},
            // top (yellow)
            {-hs.x, -hs.y, +hs.z, 0xff00ddee},
            {+hs.x, -hs.y, +hs.z, 0xff00ddee},
            {+hs.x, +hs.y, +hs.z, 0xff00ddee},
            {-hs.x, +hs.y, +hs.z, 0xff00ddee},
            // bottom (white)
            {-hs.x, +hs.y, -hs.z, 0xffffffff},
            {+hs.x, +hs.y, -hs.z, 0xffffffff},
            {+hs.x, -hs.y, -hs.z, 0xffffffff},
            {-hs.x, -hs.y, -hs.z, 0xffffffff},
        };

        // fill in user colors if provided
        if (colors.size()) {
            for (size_t i = 0, c = 0; i < 24; i += 4, ++c) {
                for (size_t j = i; j < i+4; ++j) {
                    vdata[j].abgr = colors[c % colors.size()];
                }
            }
        }

        // cw winding
        uint16_t idata[] = {
            // front
            0, 1, 2, 0, 2, 3, 
            // right
            4, 5, 6, 4, 6, 7, 
            // back
            8, 9, 10, 8, 10, 11, 
            // left
            12, 13, 14, 12, 14, 15, 
            // top
            16, 17, 18, 16, 18, 19, 
            // bottom
            20, 21, 22, 20, 22, 23, 
        };

        Mesh m;
        m.renderableKey = r->key;
        m.model = glm::translate(glm::mat4{1.f}, pos);
        m.materialId = materialId;

        memcpy(ibuffer, idata, sizeof(idata));
        auto iref = bgfx::makeRef(ibuffer, sizeof(idata));
        m.ibuf = bgfx::createIndexBuffer(iref);

        bgfx::VertexLayout layout;
        layout.begin();
        layout.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float);
        layout.add(bgfx::Attrib::Color0,   4, bgfx::AttribType::Uint8, true);
        layout.end();

        memcpy(vbuffer, vdata, sizeof(vdata));
        auto vref = bgfx::makeRef(vbuffer, sizeof(vdata));
        auto vbuf = bgfx::createVertexBuffer(vref, layout);

        // fprintf(stderr, "size of vdata %zu\n", sizeof(vdata));
        // for (size_t i = 0; i < 4; ++i) {
        //     fprintf(stderr, "(%f %f %f), 0x%08x\n", 
        //         ((Vert *)bufLoc)[i].x,
        //         ((Vert *)bufLoc)[i].y,
        //         ((Vert *)bufLoc)[i].z,
        //         ((Vert *)bufLoc)[i].abgr
        //     );
        // }

        m.vbufs.push_back(vbuf);

        r->meshes.push_back(m);

        return r->meshes.back();
    }

    inline size_t vbufferSize() {
        return sizeof(VertUnlit) * 4 * 6;
    }

    inline size_t ibufferSize() {
        return sizeof(uint16_t) * 6 * 6;
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
    // cube 0 vbuf, cube 0 ibuf, cube 1 vbuf, cube 1 ibuf, etc
    inline VertUnlit * vbufferForIndex(Renderable * r, size_t index) {
        return (VertUnlit *)(r->buffer + buffersSize(index));
    }
    inline uint16_t * ibufferForIndex(Renderable * r, size_t index) {
        return (uint16_t *)(r->buffer + buffersSize(index) + vbufferSize());
    }

    inline Mesh & create(
        Renderable * r, 
        size_t index,
        glm::vec3 const & size, 
        glm::vec3 const & pos,
        std::vector<uint32_t> const & colors = {},
        int materialId = -1
    ) {
        return create(r, vbufferForIndex(r, index), ibufferForIndex(r, index), size, pos, colors, materialId);
    }


}