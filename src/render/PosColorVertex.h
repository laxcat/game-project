#pragma once
#include "../common/types.h"


struct PosColorVertex {
    float x;
    float y;
    float z;
    uint32_t abgr;

    static void init() {
        layout
            .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0,   4, bgfx::AttribType::Uint8, true)
            .end();
    }

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

    static inline bgfx::VertexLayout layout;
};
