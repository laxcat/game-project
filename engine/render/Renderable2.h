#pragma once
#include <vector>
#include <bgfx/bgfx.h>
#include <tiny_gltf.h>
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include "../common/debug_defines.h"
#include "../memory/CharKeys.h"
#include "../memory/File.h"
#include "Mesh.h"
#include "Image.h"

// !!
// Instantiate through factory functions in RenderSystem

class Renderable final {
public:
    friend class RenderSystem;

    constexpr static size_t DataSize() { return 0; }

    enum class LoadState {Loading, WaitingToFinalize, NotLoading, FailedToLoad};

    // handled or required by rendersys
    // bool active = false;
    char key[CharKeys::KEY_MAX] = "";
    #if DEV_INTERFACE
    File::Path path;
    #endif // DEV_INTERFACE

    bgfx::ProgramHandle program;
    std::vector<Image> images;
    std::vector<bgfx::TextureHandle> textures;

    // instances. 
    // each renderable can be drawn to the screen multiple times, once for each instance.
    // this is not using GPU intancing (but it could be optimized to do so!)
    struct Instance {
        bool active = true;
        glm::mat4 model{1.f};
        glm::vec4 overrideColor = {1.f, 1.f, 1.f, 1.f};
        std::vector<size_t> ignorePointLights;
    };
    std::vector<Instance> instances{1};
    bool hasActiveInstance() { for(auto & inst : instances) { if (inst.active) return true; } return false; }
    void resetInstances(size_t count) { instances.resize(0); instances.resize(count); }

    // loader or initializer should populate these vars
    // rendersys resets them
    byte_t * buffer = nullptr;
    size_t bufferSize;
    std::vector<Mesh> meshes;
    std::vector<Mesh> meshesWithAlpha; // only used for loading. alpha meshes are at end of meshes.
    std::vector<Material> materials;
    bool isSafeToDraw = true;

    // laod state
    // designed to the the only thing that needs locking
    LoadState loadState = LoadState::NotLoading;

    // additional model adjustment
    glm::mat4 adjRotModel{1.f};
    bool adjRotAxes[3] = {false, false, false};

    void updateAdjRot() {
        adjRotModel = glm::mat4{1.f};
        if (adjRotAxes[0]) adjRotModel = glm::rotate(adjRotModel, (float)M_PI, glm::vec3{1.f, 0.f, 0.f});
        if (adjRotAxes[1]) adjRotModel = glm::rotate(adjRotModel, (float)M_PI, glm::vec3{0.f, 1.f, 0.f});
        if (adjRotAxes[2]) adjRotModel = glm::rotate(adjRotModel, (float)M_PI, glm::vec3{0.f, 0.f, 1.f});
    }

    // KEYS
    static inline char const * OriginWidgetKey          = "origin";

    // DEFAULTS
    static inline Material DefaultMaterial;
};
