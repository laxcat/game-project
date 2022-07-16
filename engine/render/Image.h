#pragma once
#include <string.h>
#include "../common/types.h"
#include <bgfx/bgfx.h>


class Image {
public:
    int width = 0;
    int height = 0;
    int comp = 0;
    byte_t * data = nullptr;
    size_t dataSize = 0;

    Image * alloc(uint16_t w, uint16_t h, uint16_t c, byte_t * newData) {
        width = w;
        height = h;
        comp = c;
        dataSize = w * h * c;
        // fprintf(stderr, "!!!!!!!!!!!!! %d %d %d\n", w, h, c);
        data = (byte_t *)malloc(dataSize);
        memcpy(data, newData, dataSize);
        return this;
    }

    void dealloc() {
        if (data) {
            free(data);
            data = nullptr;
            dataSize = 0;
        }
    }

    bgfx::Memory const * ref() const {
        return bgfx::makeRef(data, dataSize);
    }
};
