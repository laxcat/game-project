#pragma once


class UniColor {
public:
    bgfx::UniformHandle handle;
    glm::vec4 data;

    UniColor() : UniColor(0x000000ff) {};

    // asummes RGBA
    UniColor(uint32_t c) {
        data[0] = ((c >> 24) & 0xff) / 255.f;
        data[1] = ((c >> 16) & 0xff) / 255.f;
        data[2] = ((c >>  8) & 0xff) / 255.f;
        data[3] = ((c >>  0) & 0xff) / 255.f;
    }

    // from floats
    UniColor(glm::vec4 c) { data = c; }

    uint32_t asRGBAInt() const {
        return
            (byte_t(data[0] * 0xff)    << 24) | // r
            (byte_t(data[1] * 0xff)    << 16) | // g
            (byte_t(data[2] * 0xff)    <<  8) | // b
            (byte_t(data[3] * 0xff)    <<  0);  // a
    }

    uint32_t asABGRInt() const {
        return
            (byte_t(data[3] * 0xff)    << 24) | // r
            (byte_t(data[2] * 0xff)    << 16) | // g
            (byte_t(data[1] * 0xff)    <<  8) | // b
            (byte_t(data[0] * 0xff)    <<  0);  // a
    }

    glm::vec4 asVec4() const { return data; }
    operator glm::vec4 () const { return data; }

    bool operator == (UniColor const & other) {
        return other.data == data;
    }
};
