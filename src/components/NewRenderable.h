#pragma once
#include <entt/entity/entity.hpp>
#include <glm/mat4x4.hpp>


class NewRenderable {
public:
    // handled or required by manager
    // bool active = false;

    bgfx::ProgramHandle program;

    entt::entity meshFirstChild;
    size_t meshCount;

    glm::mat4 model{1.f};

    // template<class Archive>
    // void serialize(Archive & archive) {
    //     archive(active, meshFirstChild, meshCount);
    // }
};
