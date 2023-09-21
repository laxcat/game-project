#pragma once
#include <vector>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include "../common/types.h"
#include "Image.h"
#include "Material.h"

class Mesh {
public:
    class Images {
    public:
        int color = -1;
    };

    char const * renderableKey = nullptr;

    std::vector<bgfx::VertexBufferHandle> vbufs;
    bgfx::DynamicVertexBufferHandle dynvbuf = BGFX_INVALID_HANDLE;
    bgfx::IndexBufferHandle ibuf;
    Images images;

    int materialId = -1;
    glm::mat4 model{1.f};

    Material & getMaterial() const;
};
