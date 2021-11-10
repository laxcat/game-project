#include "Tester.h"
#include <stdio.h>
#include "../MrManager.h"


static uint16_t indices[] = {
    0, 1, 2,
    3, 2, 1,
    4, 5, 6,
    7, 6, 5,
    8, 9, 10,
    11, 10, 9,
    12, 13, 14,
    15, 14, 13,
    16, 17, 18,
    19, 18, 17,
    20, 21, 22,
    23, 22, 21,
};

struct PosVertex {
    float x;
    float y;
    float z;
};

static PosVertex init_verts[] = {
    {-0.5f, -0.5f,  0.5f},
    { 0.5f, -0.5f,  0.5f},
    {-0.5f,  0.5f,  0.5f},
    { 0.5f,  0.5f,  0.5f},
    { 0.5f, -0.5f,  0.5f},
    {-0.5f, -0.5f,  0.5f},
    { 0.5f, -0.5f, -0.5f},
    {-0.5f, -0.5f, -0.5f},
    { 0.5f,  0.5f,  0.5f},
    { 0.5f, -0.5f,  0.5f},
    { 0.5f,  0.5f, -0.5f},
    { 0.5f, -0.5f, -0.5f},
    {-0.5f,  0.5f,  0.5f},
    { 0.5f,  0.5f,  0.5f},
    {-0.5f,  0.5f, -0.5f},
    { 0.5f,  0.5f, -0.5f},
    {-0.5f, -0.5f,  0.5f},
    {-0.5f,  0.5f,  0.5f},
    {-0.5f, -0.5f, -0.5f},
    {-0.5f,  0.5f, -0.5f},
    {-0.5f, -0.5f, -0.5f},
    {-0.5f,  0.5f, -0.5f},
    { 0.5f, -0.5f, -0.5f},
    { 0.5f,  0.5f, -0.5f},
};

static PosVertex init_norms[] = {
    { 0.f,  0.f,  1.f},
    { 0.f,  0.f,  1.f},
    { 0.f,  0.f,  1.f},
    { 0.f,  0.f,  1.f},
    { 0.f, -1.f,  0.f},
    { 0.f, -1.f,  0.f},
    { 0.f, -1.f,  0.f},
    { 0.f, -1.f,  0.f},
    { 1.f,  0.f,  0.f},
    { 1.f,  0.f,  0.f},
    { 1.f,  0.f,  0.f},
    { 1.f,  0.f,  0.f},
    { 0.f,  1.f,  0.f},
    { 0.f,  1.f,  0.f},
    { 0.f,  1.f,  0.f},
    { 0.f,  1.f,  0.f},
    {-1.f,  0.f,  0.f},
    {-1.f,  0.f,  0.f},
    {-1.f,  0.f,  0.f},
    {-1.f,  0.f,  0.f},
    { 0.f,  0.f, -1.f},
    { 0.f,  0.f, -1.f},
    { 0.f,  0.f, -1.f},
    { 0.f,  0.f, -1.f},
};


void Tester::init() {
    renderable = mm.rendMan.create(mm.mem.loadProgram("vs_tester", "fs_tester"), "tester");
    Renderable * r = mm.rendMan.at(renderable);

    r->meshes.push_back({});
    auto & mesh = r->meshes.back();

    {
        bgfx::VertexLayout layout;
        layout
            .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .end();
        auto ref = bgfx::makeRef(init_verts, sizeof(init_verts));
        auto vbuf = bgfx::createVertexBuffer(ref, layout);
        mesh.vbufs.push_back(vbuf);
    }

    {
        bgfx::VertexLayout layout;
        layout
            .begin()
            .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
            .end();
        auto ref = bgfx::makeRef(init_norms, sizeof(init_norms));
        auto vbuf = bgfx::createVertexBuffer(ref, layout);
        mesh.vbufs.push_back(vbuf);
    }

    mesh.ibuf = bgfx::createIndexBuffer(bgfx::makeRef(indices, sizeof(indices)));
}

void Tester::shutdown() {
    if (renderable == -1) return;
    auto & meshes = mm.rendMan.at(renderable)->meshes;
    free(meshes[0].verts);
    meshes[0].verts = nullptr;
    renderable = -1;
}
