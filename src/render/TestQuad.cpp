#include "TestQuad.h"
#include <stdio.h>
#include "../MrManager.h"


static uint16_t indices[6] = {
    0, 1, 2,
    0, 2, 3,
};

static PosColorVertex init_verts[] = {
    {0.f, 0.f, 0.f, 0xffff0000},
    {1.f, 0.f, 0.f, 0xff00ff00},
    {1.f, 1.f, 0.f, 0xff0000ff},
    {0.f, 1.f, 0.f, 0xffff00ff},
};

void TestQuad::init() {
    program = mm.mem.loadProgram("vs_main", "fs_main");

    // important to place mesh in place first and get a reference
    meshes.push_back({});
    auto & mesh = meshes.back();

    mesh.initVerts(4);
    memcpy(mesh.verts, init_verts, mesh.vertsByteSize);

    for (int i = 0; i < 4; ++i) {
        printf("vert %d: %08x\n", i, mesh.verts[i].abgr);
    }

    mesh.dvbufs.push_back(bgfx::createDynamicVertexBuffer(mesh.vertsRef(), PosColorVertex::layout));
    mesh.ibuf = bgfx::createIndexBuffer(bgfx::makeRef(indices, sizeof(indices)));
}

// void TestQuad::draw() {
    // bgfx::update(meshes[0].dvbufs[0], 0, meshes[0].vertsRef());
    // bgfx::setVertexBuffer(mm.mainView, meshes[0].dvbufs[0]);
    // bgfx::setIndexBuffer(meshes[0].ibuf);
    // bgfx::setState(BGFX_STATE_WRITE_RGB);
    // bgfx::submit(mm.mainView, program);
// }

void TestQuad::shutdown() {
    free(meshes[0].verts);
    meshes[0].verts = nullptr;
}
