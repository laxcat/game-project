#pragma once
#include "Image.h"
#include <bgfx/bgfx.h>
#include "../dev/print.h"


class Texture {
public:
    Image img;
    bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;

    // if data is not null, texture/image will be immutable
    bgfx::TextureHandle createMutable(
        uint16_t w, uint16_t h, uint16_t comp,
        uint64_t flags = BGFX_TEXTURE_NONE|BGFX_SAMPLER_NONE
    ) {
        // TODO only check during debug?
        if (!w && !h && !comp) { print("WARNING: nothing to create!"); return BGFX_INVALID_HANDLE; }

        img.alloc(w, h, comp, nullptr);
        createTexture(flags, nullptr);
        return handle;
    }

    bgfx::TextureHandle createImmutable(
        uint16_t w, uint16_t h, uint16_t comp,
        byte_t * data,
        uint64_t flags = BGFX_TEXTURE_NONE|BGFX_SAMPLER_NONE
    ) {
        // TODO only check during debug?
        if (!w && !h && !comp) { print("WARNING: nothing to create!"); return BGFX_INVALID_HANDLE; }

        img.alloc(w, h, comp, data);
        createTexture(flags, data);
        return handle;
    }

    void update() {
        // TODO only check during debug?
        if (!isValid(handle)) { print("WARNING: texture not created yet, cannot update!"); return; }

        bgfx::updateTexture2D(handle, 1, 0, 0, 0, img.width, img.height, img.ref());
    }

    void destroy() {
        img.dealloc();
        bgfx::destroy(handle);
    }

    void setPixel(size_t i, uint32_t val) { img.setPixel(i, val); update(); }
    void setPixel(int x, int y, uint32_t val) { img.setPixel(x, y, val); update(); }
    void fill(uint32_t val) { img.fill(val); update(); }
    void fill(float * val) { img.fill(val); update(); }
    void fillCheckered(uint32_t a, uint32_t b) { img.fillCheckered(a, b); update(); }
    void fillCheckered(float * a, float * b){ img.fillCheckered(a, b); update(); }

private:
    void createTexture(uint64_t flags = BGFX_TEXTURE_NONE|BGFX_SAMPLER_NONE, void * data = nullptr) {
        // TODO only check during debug?
        if (isValid(handle)) { print("WARNING: already created!"); }

        handle = bgfx::createTexture2D(
            uint16_t(img.width),
            uint16_t(img.height),
            false,
            1,
                (img.comp == 4) ? bgfx::TextureFormat::RGBA8 :
                (img.comp == 3) ? bgfx::TextureFormat::RGB8 :
                (img.comp == 2) ? bgfx::TextureFormat::RG8 :
                (img.comp == 1) ? bgfx::TextureFormat::R8 :
                bgfx::TextureFormat::Count,
            flags,
            (data) ? img.ref() : nullptr
        );
    }
};
