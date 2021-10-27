#pragma once
#include "Renderable.h"
#include "PosColorVertex.h"


class TestQuad : public Renderable {
public:

    static size_t const vertCount = 4;

    PosColorVertex verts[TestQuad::vertCount] = {
        {0.f, 0.f, 0.f, 0xffff0000},
        {1.f, 0.f, 0.f, 0xff00ff00},
        {1.f, 1.f, 0.f, 0xff0000ff},
        {0.f, 1.f, 0.f, 0xffff00ff},
    };

    float vertFloatColors[vertCount*4];

    void init() override;
    void draw() override;

};
