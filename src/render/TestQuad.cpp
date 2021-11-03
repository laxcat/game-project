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
    primitives.push_back({});
    auto & primitive = primitives.back();

    primitive.initVerts(4);
    memcpy(primitive.verts, init_verts, primitive.vertsByteSize);

    for (int i = 0; i < 4; ++i) {
        printf("vert %d: %08x\n", i, primitive.verts[i].abgr);
    }

    primitive.vbh = bgfx::createDynamicVertexBuffer(primitive.vertsRef(), PosColorVertex::layout);
    primitive.ibh = bgfx::createIndexBuffer(bgfx::makeRef(indices, sizeof(indices)));
}

void TestQuad::draw() {
    bgfx::update(primitives[0].vbh, 0, primitives[0].vertsRef());
    bgfx::setVertexBuffer(mm.mainView, primitives[0].vbh);
    bgfx::setIndexBuffer(primitives[0].ibh);
    bgfx::setState(BGFX_STATE_WRITE_RGB);
    bgfx::submit(mm.mainView, mm.program);
}

void TestQuad::shutdown() {
    free(primitives[0].verts);
    primitives[0].verts = nullptr;
}
