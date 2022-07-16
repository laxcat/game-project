#pragma once
#include "Image.h"
#include <bgfx/bgfx.h>


class Texture {
public:
    Image * img = nullptr;
    Image exclusiveImage;
    bgfx::TextureHandle handle;

    bgfx::TextureHandle attachImageAndCreate(Image & sharedImg) {
        img = &sharedImg;
        return createTexture2D();
    }

    bgfx::TextureHandle allocImageAndCreate(uint16_t w, uint16_t h, uint16_t comp, byte_t * newData) {
        exclusiveImage.dealloc();
        img = &exclusiveImage;
        img->alloc(w, h, comp, newData);
        return createTexture2D();
    }

    bgfx::TextureHandle createTexture2D() {
        handle = bgfx::createTexture2D(
            uint16_t(img->width), 
            uint16_t(img->height), 
            false, 
            1, 
            bgfx::TextureFormat::RGBA8, 
            0, 
            img->ref()
        );
        return handle;
    }

    void destroy() {
        if (img) {
            img->dealloc();
        }
        bgfx::destroy(handle);
    }
};