#pragma once
#include "../render/Renderable.h"
#include "../shader/shaders/main/VertMain.h"


inline Mesh & createPyramid(
        Renderable * r, 
        VertMain * vbuffer, 
        uint16_t * ibuffer, 
        glm::vec3 size,
        std::vector<uint32_t> const & colors
    ) {
    auto hs = size * .5f;
    // y-up
    VertMain vdata[] = {
        // front (blue)
        {-hs.x,       0, +hs.z, 0xffff0000},
        {+hs.x,       0, +hs.z, 0xffff0000},
        {    0,  size.y,     0, 0xffff0000},
        // right (red)
        {+hs.x,       0, +hs.z, 0xff0000ff},
        {+hs.x,       0, -hs.z, 0xff0000ff},
        {    0,  size.y,     0, 0xff0000ff},
        // back (green)
        {+hs.x,       0, -hs.z, 0xff00ff00},
        {-hs.x,       0, -hs.z, 0xff00ff00},
        {    0,  size.y,     0, 0xff00ff00},
        // left (orange)
        {-hs.x,       0, -hs.z, 0xff2288dd},
        {-hs.x,       0, +hs.z, 0xff2288dd},
        {    0,  size.y,     0, 0xff2288dd},
        // base (yellow)
        {-hs.x,       0, -hs.z, 0xff00ddee},
        {+hs.x,       0, -hs.z, 0xff00ddee},
        {+hs.x,       0, +hs.z, 0xff00ddee},
        {-hs.x,       0, +hs.z, 0xff00ddee},
    };

    // set the actual colors.
    // color the sides, rotating through the colors
    for (size_t i = 0, c = 0; i < 15; i += 3, ++c) {
        for (size_t j = i; j < i+3; ++j) {
            vdata[j].abgr = colors[c % colors.size()];
        }
    }
    // color the last vertex in the base
    vdata[15].abgr = vdata[14].abgr;

    // cw winding
    uint16_t idata[] = {
        // front
        0, 1, 2, 
        // right
        3, 4, 5,
        // back
        6, 7, 8,
        // left
        9, 10, 11,
        // bottom
        12, 13, 14, 12, 14, 15, 
    };

    Mesh m;
    // m.model = glm::translate(glm::mat4{1.f}, pos);

    memcpy(ibuffer, idata, sizeof(idata));
    auto iref = bgfx::makeRef(ibuffer, sizeof(idata));
    m.ibuf = bgfx::createIndexBuffer(iref);

    // TODO: move layout to more central location
    bgfx::VertexLayout layout;
    layout.begin();
    layout.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float);
    layout.add(bgfx::Attrib::Color0,   4, bgfx::AttribType::Uint8, true);
    layout.end();

    memcpy(vbuffer, vdata, sizeof(vdata));
    auto vref = bgfx::makeRef(vbuffer, sizeof(vdata));
    auto vbuf = bgfx::createVertexBuffer(vref, layout);

    m.vbufs.push_back(vbuf);

    r->meshes.push_back(m);

    return r->meshes.back();
}
