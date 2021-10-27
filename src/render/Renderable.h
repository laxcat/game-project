#pragma once
#include <bgfx/bgfx.h>


class Renderable {
public:

    bgfx::DynamicVertexBufferHandle vbh;
    bgfx::Memory const * vbRef;
    bgfx::IndexBufferHandle ibh;


    virtual void init() {}
    virtual void draw() {}
};
