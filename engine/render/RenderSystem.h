#pragma once
#include "Colors.h"
#include "Fog.h"
#include "Lights.h"
#include "RenderSettings.h"
#include "Samplers.h"
#include "Texture.h"
#include "../common/debug_defines.h"
#include "../memory/MemMan.h"
#include "../memory/Gobj.h"

class RenderSystem {
public:
    //
    // SHARED RESOURCES
    //
    Samplers samplers;
    bgfx::ProgramHandle unlitProgram;
    bgfx::ProgramHandle gltfProgram;
    Lights lights;
    Fog fog;
    Colors colors;
    RenderSettings settings;

    // 
    // MANAGER LIFECYCLE
    //
    void init();
    // void draw();
    void draw();
    // returns submit count
    uint16_t drawMesh(Gobj::Mesh const & mesh);
    void shutdown();

    // 
    // RENDERABLE LIFECYCLE
    //
    // Renderable * create(bgfx::ProgramHandle program, char const * key);
    // Renderable * createFromGLTF(char const * filename, char const * key);
    // Renderable * at(char const * key);

    void add(char const * key, Gobj * g);
    void remove(char const * key);
    // returns gobj at key before update, nullptr if not found
    Gobj * update(char const * key, Gobj * newGobj);

    // 
    // RENDERABLE UTILS
    //
    bool keyExists(char const * key);
    // bool isKeySafeToDrawOrLoad(char const * key);
    // bool isRenderableLoading(Renderable const * r, Renderable::LoadState & foundLoadState);

    #if DEBUG || DEV_INTERFACE
    void showStatus();
    void showMoreStatus(char const * prefix = "");
    #endif // DEBUG || DEV_INTERFACE

    size_t renderableCount() const;

private:
    // GLTFLoader gltfLoader;
    // std::unordered_map<Renderable const *, std::thread> loadingThreads;
    bgfx::UniformHandle materialBaseColor;
    bgfx::UniformHandle materialPBRValues;
    bgfx::UniformHandle normModel;
    Texture whiteTexture;
    BXAllocator bxAllocator;

    CharKeys * renderList = nullptr;
    
    void addHandles(Gobj * g);
    void removeHandles(Gobj * g);

    // bool destroy(char const * key);
    // void reset(Renderable * r);

    #if DEV_INTERFACE
    friend class Editor;
    #endif // DEV_INTERFACE
};
