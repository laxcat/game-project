#pragma once
#include <stdio.h>
#include <filesystem>
#include <vector>
#include <glm/mat4x4.hpp>
#include "types.h"


// convert vector of doubles to glm::mat4 type
inline void glmMat4(glm::mat4 m, std::vector<double> const & v) {
    if (v.size() == 16) {
        for (int i = 0; i < 16; ++i) {
            m[i/4][i%4] = (float)v[i];
        }
    }
}
inline glm::mat4 glmMat4(std::vector<double> const & v) {
    glm::mat4 m{1.f};
    glmMat4(m, v);
    return m;
}

inline glm::vec4 glmVec4(std::vector<double> const & v) {
    return glm::vec4{v[0], v[1], v[2], v[3]};
}

inline char const * relPath(char const * path) {
    auto rel = std::filesystem::relative(path);
    static char ret[1024];
    strcpy(ret, rel.c_str());
    return ret;
}


inline void colorIntToVec4(uint32_t c, glm::vec4 & out) {
    out[0] = ((c >> 24)       ) / 255.f;
    out[1] = ((c >> 16) & 0xff) / 255.f;
    out[2] = ((c >>  8) & 0xff) / 255.f;
    out[3] = ( c        & 0xff) / 255.f;
}
inline glm::vec4 colorIntToVec4(uint32_t c) {
    glm::vec4 ret;
    colorIntToVec4(c, ret);
    return ret;
}

inline int colorVec4ToInt(glm::vec4 const & c) {
    int ret = 
        (int(c.r * 255.f) << 24) |
        (int(c.g * 255.f) << 16) |
        (int(c.b * 255.f) <<  8) |
        (int(c.a * 255.f) <<  0);
    return ret;
}

inline float randFloat() {
    return (float)rand()/(float)RAND_MAX;
}

inline float remapValue(float value, float min1, float max1, float min2, float max2) {
  return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

inline float lerp(float x0, float x1, float v) {
    return x0 + (x1 - x0) * v;
}

// inline bool withinRange(float a, float b, float maxDiff = 0.05) {
//     return (fabsf(a, b) <= maxDiff);
// }