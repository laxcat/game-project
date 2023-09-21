#pragma once
#include <glm/vec4.hpp>


class Material {
public:
    char name[32];

    glm::vec4 baseColor = {1.f, 1.f, 1.f, 1.f}; // rgba

    glm::vec4 pbrValues = {
        0.5f, // roughness. 0 smooth, 1 rough
        0.0f, // metallic. 0 plastic, 1 metal
        0.2f, // specular, additional specular adjustment for non-matalic materials
        1.0f  // color intensity
    };
    float & roughness() { return pbrValues[0]; }
    float & metallic() { return pbrValues[1]; }
    float & specular() { return pbrValues[2]; }
    float & baseColorIntensity() { return pbrValues[3]; }
};
