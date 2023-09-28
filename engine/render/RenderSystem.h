#pragma once
#include <bimg/bimg.h>
#include "Colors.h"
#include "Fog.h"
#include "Lights.h"
#include "RenderSettings.h"
#include "Samplers.h"
#include "../common/debug_defines.h"
#include "../memory/MemMan.h"
#include "../memory/Gobj.h"

class RenderSystem {
public:
    Samplers samplers;
    bgfx::ProgramHandle unlitProgram;
    bgfx::ProgramHandle standardProgram;
    Lights lights;
    Fog fog;
    Colors colors;
    RenderSettings settings;

    void init();
    void draw();
    // returns aggregate submit count
    uint16_t drawNode(Gobj * gobj, Gobj::Node * node, glm::mat4 const & parentTransform = Identity);
    // returns submit count
    uint16_t drawMesh(Gobj * gobj, Gobj::Mesh const & mesh, glm::mat4 const & transform = Identity);
    void shutdown();

    void add(char const * key, Gobj * gobj);
    void remove(char const * key);
    // returns gobj at key before update, nullptr if not found
    Gobj * update(char const * key, Gobj * newGobj);

    bool keyExists(char const * key);
    Gobj * gobjForKey(char const * key);

    #if DEBUG || DEV_INTERFACE
    void showStatus();
    void showMoreStatus(char const * prefix = "");
    #endif // DEBUG || DEV_INTERFACE

    size_t renderableCount() const;

private:
    bgfx::UniformHandle materialBaseColor;
    bgfx::UniformHandle materialPBRValues;
    bgfx::UniformHandle normModel;
    bgfx::TextureHandle whiteTexture;
    BXAllocator bxAllocator;

    CharKeys * renderList = nullptr;
    
    void addHandles(Gobj * gobj);
    bimg::ImageContainer * decodeImage(Gobj::Image * img);
    void removeHandles(Gobj * gobj);

    #if DEV_INTERFACE
    friend class Editor;
    #endif // DEV_INTERFACE

    static constexpr glm::mat4 const Identity = glm::mat4{1.f};
};

    // bool destroy(char const * key);
    // void reset(Renderable * r);

    // GLTFLoader gltfLoader;
    // std::unordered_map<Renderable const *, std::thread> loadingThreads;

    // bool isKeySafeToDrawOrLoad(char const * key);
    // bool isRenderableLoading(Renderable const * r, Renderable::LoadState & foundLoadState);

    //
    // RENDERABLE LIFECYCLE
    //
    // Renderable * create(bgfx::ProgramHandle program, char const * key);
    // Renderable * createFromGLTF(char const * filename, char const * key);
    // Renderable * at(char const * key);

