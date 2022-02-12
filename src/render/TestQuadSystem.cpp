#include "TestQuadSystem.h"
#include <stdio.h>
#include "../MrManager.h"


static uint16_t indices[6] = {
    0, 1, 2,
    0, 2, 3,
};

struct PosColorVertex {
    float x;
    float y;
    float z;
    uint32_t abgr;

    float * getColor3(float * color) {
        color[0] = ((abgr >>  0) & 0xff) / 255.f; // r
        color[1] = ((abgr >>  8) & 0xff) / 255.f; // g
        color[2] = ((abgr >> 16) & 0xff) / 255.f; // b
        return color;
    }

    void setColor3(float * color) {
        abgr = 
            (((abgr >> 24) & 0xff)      << 24) | // a
            (byte_t(color[2] * 0xff)    << 16) | // b
            (byte_t(color[1] * 0xff)    <<  8) | // g
            (byte_t(color[0] * 0xff)    <<  0);  // r

    }
};

static PosColorVertex init_verts[] = {
    {-0.5f, -0.5f, 0.f, 0xffff0000},
    {+0.5f, -0.5f, 0.f, 0xff00ff00},
    {+0.5f, +0.5f, 0.f, 0xff0000ff},
    {-0.5f, +0.5f, 0.f, 0xffff00ff},
};

size_t TestQuadSystem::renderable = -1;

void TestQuadSystem::init() {
    auto prog = mm.memSys.loadProgram("vs_main", "fs_main");
    renderable = mm.rendSys.create(prog, "testquad");
    Renderable * r = mm.rendSys.at(renderable);

    // important to place mesh in place first and get a reference
    r->meshes.push_back({});
    auto & mesh = r->meshes.back();

    mesh.initVerts(4, sizeof(PosColorVertex));
    memcpy(mesh.verts, init_verts, mesh.vertsTotalByteSize);

    for (int i = 0; i < 4; ++i) {
        print("vert %d: %08x\n", i, mesh.vertAt<PosColorVertex>(i)->abgr);
    }

    bgfx::VertexLayout layout;
    layout
        .begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0,   4, bgfx::AttribType::Uint8, true)
        .end();

    mesh.dvbufs.push_back(bgfx::createDynamicVertexBuffer(mesh.vertsRef(), layout));
    mesh.ibuf = bgfx::createIndexBuffer(bgfx::makeRef(indices, sizeof(indices)));




    // auto & reg = mm.registry;
    // auto e = mm.rendSys.create2(prog, "testquad2");
    // NewRenderable & renderable = reg.get<NewRenderable>(e);
    // renderable.meshFirstChild = reg.create();
    // renderable.meshCount = 1;

    // NewMesh & newMesh = reg.emplace<NewMesh>(renderable.meshFirstChild);

    // auto ref = bgfx::makeRef(init_verts, sizeof(init_verts));
    // auto vbuf = bgfx::createDynamicVertexBuffer(ref, layout);
    // newMesh.dvbufRefs.push_back(ref);
    // newMesh.dvbufs.push_back(vbuf);
    // newMesh.ibuf = bgfx::createIndexBuffer(bgfx::makeRef(indices, sizeof(indices)));

}

void TestQuadSystem::shutdown() {
    if (renderable == -1) return;
    auto & meshes = mm.rendSys.at(renderable)->meshes;
    free(meshes[0].verts);
    meshes[0].verts = nullptr;
    renderable = -1;
}

float * TestQuadSystem::getColor3(int index, float * color) {
    return mm.rendSys.at(renderable)->meshes[0].vertAt<PosColorVertex>(index)->getColor3(color);
}

void TestQuadSystem::setColor3(int index, float * color) {
    return mm.rendSys.at(renderable)->meshes[0].vertAt<PosColorVertex>(index)->setColor3(color);
}

