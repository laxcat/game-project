#pragma once
#include "Renderable.h"
#include "PosColorVertex.h"


class TestQuad : public Renderable {
public:
    void init() override;
    void draw() override;
    void shutdown() override;
};
