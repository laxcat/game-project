#pragma once
#include <string.h>
#include "../common/types.h"
#include "../common/utils.h"
#include "../dev/print.h"
#include <bgfx/bgfx.h>


class Image {
public:
    int width = 0;
    int height = 0;
    int comp = 0;
    int pixelCount = 0;
    byte_t * data = nullptr;
    size_t dataSize = 0;

    Image * alloc(uint16_t w, uint16_t h, uint16_t c, byte_t * newData) {
        width = w;
        height = h;
        comp = c;
        pixelCount = w * h;
        dataSize = pixelCount * c;
        // fprintf(stderr, "!!!!!!!!!!!!! %d %d %d\n", w, h, c);
        data = (byte_t *)malloc(dataSize);
        if (newData) {
            memcpy(data, newData, dataSize);
        }
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

    // val is int-color as 0xRRGGBBAA
    void setPixel(size_t i, uint32_t val) {
        data[i*comp+0] = ((byte_t *)&val)[3];
        data[i*comp+1] = ((byte_t *)&val)[2];
        data[i*comp+2] = ((byte_t *)&val)[1];
        data[i*comp+3] = ((byte_t *)&val)[0];
        // print("setting %zu to 0x%08x\n", i, val);
    }

    void setPixel(int x, int y, uint32_t val) {
        if (x >= width || y >= height) { print("WARNING: out of range.\n"); return; }
        setPixel(x + y * width, val);
    }

    // val is int-colors as 0xRRGGBBAA
    void fill(uint32_t val) {
        for (size_t i = 0; i < pixelCount; ++i) {
            setPixel(i, val);
        }
    }

    // val is int-colors as 0xRRGGBBAA
    void fill(float * a) {
        fill(colorVec4ToUint(a));
    }

    // a/b are int-colors as 0xRRGGBBAA
    void fillCheckered(uint32_t a, uint32_t b) {
        for (size_t i = 0; i < pixelCount; ++i) {
            bool isB = (i % width + i / width) % 2;
            setPixel(i, (isB) ? b : a);
        }
    }

    // a/b are 4 component colors (RGBA), each component is 0-1 float value
    void fillCheckered(float * a, float * b){
        uint32_t aa = colorVec4ToUint(a);
        uint32_t bb = colorVec4ToUint(b);
        // print("A(%0.5f %0.5f %0.5f %0.5f), 0x%08x\n", a[0], a[1], a[2], a[3], aa);
        // print("B(%0.5f %0.5f %0.5f %0.5f), 0x%08x\n", b[0], b[1], b[2], b[3], bb);
        fillCheckered(aa, bb);
    }
};
