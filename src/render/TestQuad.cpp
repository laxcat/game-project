#include "TestQuad.h"
#include <stdio.h>
#include "../MrManager.h"


static uint16_t indices[6] = {
    0, 1, 2,
    0, 2, 3,
};


void TestQuad::init() {
    for (int i = 0; i < 4; ++i) {
        printf("vert %d: %08x\n", i, verts[i].abgr);
    }

    vbRef = bgfx::makeRef(verts, sizeof(verts));
    vbh = bgfx::createDynamicVertexBuffer(vbRef, PosColorVertex::layout);
    ibh = bgfx::createIndexBuffer(bgfx::makeRef(indices, sizeof(indices)));
}

void TestQuad::draw() {
    bgfx::update(vbh, 0, bgfx::makeRef(verts, sizeof(verts)));
    bgfx::setVertexBuffer(mm.mainView, vbh);
    bgfx::setIndexBuffer(ibh);
    bgfx::setState(BGFX_STATE_WRITE_RGB);
    bgfx::submit(mm.mainView, mm.program);
}