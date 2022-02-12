#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <entt/entity/registry.hpp>
#include "Colors.h"
#include "GLTFLoader.h"
#include "Renderable.h"


class RenderSystem {
public:
    Colors colors;

    // 
    // MANAGER LIFECYCLE
    //

    void init();
    void draw();
    void shutdown();

    // 
    // RENDERABLE LIFECYCLE
    //

    size_t create(bgfx::ProgramHandle program, char const * key = "");
    entt::entity create2(bgfx::ProgramHandle program, char const * key = "");

    size_t createFromGLTF(char const * filename = "");
    Renderable * at(int index);
    Renderable * at(char const * key);
    void destroy(size_t index);
    void destroy(char const * key);
    // 
    // RENDERABLE UTILS
    //

    bool keyExists(char const * key);


private:

    void setPoolNextFree(size_t startFrom);

    static constexpr size_t poolSize = 16;
    Renderable pool[poolSize];
    size_t poolNextFree = 0;
    std::vector<size_t> renderables;
    std::unordered_map<std::string, size_t> keys;
    GLTFLoader gltfLoader;
    bgfx::ProgramHandle gltfProgram;
};
