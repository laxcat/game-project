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
    vertCount = 4;

    vertsByteSize = sizeof(PosColorVertex) * vertCount;
    printf("vertsByteSize %zu\n", vertsByteSize);
    verts = (PosColorVertex *)malloc(vertsByteSize);
    memcpy(verts, init_verts, vertsByteSize);

    for (int i = 0; i < 4; ++i) {
        printf("vert %d: %08x\n", i, verts[i].abgr);
    }

    vbh = bgfx::createDynamicVertexBuffer(vertsRef(), PosColorVertex::layout);
    ibh = bgfx::createIndexBuffer(bgfx::makeRef(indices, sizeof(indices)));
}

void TestQuad::draw() {
    bgfx::update(vbh, 0, vertsRef());
    bgfx::setVertexBuffer(mm.mainView, vbh);
    bgfx::setIndexBuffer(ibh);
    bgfx::setState(BGFX_STATE_WRITE_RGB);
    bgfx::submit(mm.mainView, mm.program);
}

void TestQuad::shutdown() {
    free(verts);
    verts = nullptr;
}
