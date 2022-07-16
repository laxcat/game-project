#pragma once
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include "Colors.h"
#include "Fog.h"
#include "GLTFLoader.h"
#include "Lights.h"
#include "Renderable.h"
#include "RenderSettings.h"
#include "Samplers.h"
#include "Texture.h"
#include "../common/KeyedPool.h"


using RenderableIterator = KeyedPool<std::string, Renderable>::Iterator;

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
    void draw();
    void shutdown();

    // 
    // RENDERABLE LIFECYCLE
    //
    Renderable * create(bgfx::ProgramHandle program, char const * key);
    Renderable * createFromGLTF(char const * filename, char const * key);
    Renderable * at(char const * key);

    // 
    // RENDERABLE UTILS
    //
    bool keyExists(char const * key);
    bool isKeySafeToDrawOrLoad(char const * key);
    bool isRenderableLoading(Renderable const & r, Renderable::LoadState & foundLoadState);
    void showStatus();
    void showMoreStatus(char const * prefix = "");

    size_t renderableCount() const { return renderablePool.size(); };

private:
    KeyedPool<std::string, Renderable> renderablePool;
    GLTFLoader gltfLoader;
    std::unordered_map<Renderable *, std::thread> loadingThreads;
    bgfx::UniformHandle materialBaseColor;
    bgfx::UniformHandle materialPBRValues;
    bgfx::UniformHandle normModel;
    Texture whiteTexture;
    
    RenderableIterator destroy(char const * key);
    void reset(char const * key);
};
