#include "RenderSystem2.h"
#include <bgfx/platform.h>
#include "../MrManager.h"
#include "../common/string_utils.h"
#include "../render/bgfx_extra.h"

#if FORCE_OPENGL
    #include "../shader/shaders/gltf/vs_gltf.150.bin.geninc"
    #include "../shader/shaders/gltf/fs_gltf.150.bin.geninc"
    #include "../shader/shaders/unlit/vs_unlit.150.bin.geninc"
    #include "../shader/shaders/unlit/fs_unlit.150.bin.geninc"
#else
    #include "../shader/shaders/gltf/vs_gltf.metal.bin.geninc"
    #include "../shader/shaders/gltf/fs_gltf.metal.bin.geninc"
    #include "../shader/shaders/unlit/vs_unlit.metal.bin.geninc"
    #include "../shader/shaders/unlit/fs_unlit.metal.bin.geninc"
#endif

static bool constexpr ShowRenderDbg = true;
static bool constexpr ShowRenderDbgTick = ShowRenderDbg && false;

// 
// MANAGER LIFECYCLE
//

void RenderSystem::init() {
    // ensure single thread rendering
    bgfx::renderFrame();

    settings.init(mm.window, mm.windowSize, mm.setup.forceOpenGL);
    bxAllocator.memMan = &mm.memMan;
    settings.bgfxInit.allocator = &bxAllocator;
    if (!bgfx::init(settings.bgfxInit))
        return;

    gltfProgram = CREATE_BGFX_PROGRAM(gltf);
    unlitProgram = CREATE_BGFX_PROGRAM(unlit);
    samplers.init();
    lights.init();
    fog.init();
    colors.init();
    materialBaseColor = bgfx::createUniform("u_materialBaseColor", bgfx::UniformType::Vec4);
    materialPBRValues = bgfx::createUniform("u_materialPBRValues",  bgfx::UniformType::Vec4);
    normModel = bgfx::createUniform("u_normModel", bgfx::UniformType::Mat3);

    pool = mm.memMan.createCharKeys(8);

    byte_t data[] = {
        255,255,255,255,
        255,255,255,255,
        255,255,255,255,
        255,255,255,255
    };
    whiteTexture.createImmutable(2, 2, 4, data);
}

void RenderSystem::draw() {
    printc(ShowRenderDbgTick, "STARTING RENDER FRAME-----------------------------------------------------------\n");

    lights.preDraw();
    mm.camera.preDraw();
    bgfx::setUniform(fog.handle, (float *)&fog.data);
    bgfx::setUniform(colors.background.handle, (float *)&colors.background.data);

    size_t submitCount = 0;

    // for each renderable
    for (auto node : pool) {
        auto & r = *(Renderable *)node->ptr;

        // skip if ready to draw but not active
        if (!r.hasActiveInstance() && r.isSafeToDraw) {
            continue;
        }

        printc(ShowRenderDbgTick, 
            "----------------------------------------\n"
            "RENDERABLE %s (%p)\n"
            "----------------------------------------\n",
            r.key, r.key
        );

        // is loading
        if (!r.isSafeToDraw) {
            // if renderable is loading, skip draw
            auto foundLoadState = Renderable::LoadState::NotLoading;
            if (isRenderableLoading(r, foundLoadState)) {
                continue;
            }
            // done loading, finalize renderable, join threads
            else if (foundLoadState == Renderable::LoadState::WaitingToFinalize) {
                gltfLoader.loadingMutex.lock();
                r.loadState = Renderable::LoadState::NotLoading;
                gltfLoader.loadingMutex.unlock();
                r.isSafeToDraw = true;

                loadingThreads.at(&r).join();
                loadingThreads.erase(&r);
                printc(ShowRenderDbg, "Finished loading renderable %p\n", &r);
            }
            else if (foundLoadState == Renderable::LoadState::FailedToLoad) {
                printc(ShowRenderDbg, "FAILING TO LOAD\n");
                loadingThreads.at(&r).join();
                loadingThreads.erase(&r);
                destroy(r.key);
                continue;
            }
        }
        // !!!
        // IMPORTANT FOR THREAD SAFETY
        // should not make it to this point if renderable is still loading
        // !!!
        // r.loadState is guanteed Renderable::LoadState::NotLoading (or FailedToLoad) at this point
        // !!!

        // draw model instances... not using GPU intancing (but it could be optimized to do so!)
        for (size_t i = 0, e = r.instances.size(); i < e; ++i) {
            auto const & instance = r.instances[i];
            // skip if not active
            if (!instance.active) continue;

            printc(ShowRenderDbgTick, 
                "--------------------\n"
                "instance %zu\n"
                "--------------------\n",
                i
            );

            // draw meshes
            for (auto const & mesh : r.meshes) {

                printc(ShowRenderDbgTick, 
                    "----------\n"
                    "mesh %p\n"
                    "----------\n",
                    &mesh
                );

                if constexpr (ShowRenderDbg) {
                    if (mesh.renderableKey == nullptr || strcmp(mesh.renderableKey, r.key) != 0) {
                        print("WARNING! Key not set on mesh in %s\n", r.key);
                    }
                }

                // set buffers
                if (isValid(mesh.dynvbuf)) {
                    bgfx::setVertexBuffer(0, mesh.dynvbuf);
                }
                else {
                    for (size_t i = 0; i < mesh.vbufs.size(); ++i) {
                        bgfx::setVertexBuffer(i, mesh.vbufs[i]);
                    }
                }
                bgfx::setIndexBuffer(mesh.ibuf);
                bgfx::setState(mm.rendSys.settings.state);

                // set textures
                if (mesh.images.color >= 0) {
                    bgfx::setTexture(0, samplers.color, r.textures[mesh.images.color]);
                }
                else {
                    bgfx::setTexture(0, samplers.color, whiteTexture.handle);
                }
                auto & material = mesh.getMaterial();
                glm::vec4 color = material.baseColor;
                if (instance.overrideColor != glm::vec4{1.f, 1.f, 1.f, 1.f}) {
                    color = instance.overrideColor;
                }
                bgfx::setUniform(materialBaseColor, &color);
                bgfx::setUniform(materialPBRValues, &material.pbrValues);

                // get this meshes's final model
                auto model = instance.model * r.adjRotModel * mesh.model;
                bgfx::setTransform((float *)&model);

                // make a reduced version of the rotation for the shader normals
                auto nm = glm::transpose(glm::inverse(glm::mat3{model}));
                bgfx::setUniform(normModel, (float *)&nm);

                // disable ignored lights
                lights.ignorePointLights(instance.ignorePointLights);

                // submit
                bgfx::submit(mm.mainView, r.program);
                ++submitCount;

                // restor any ignored lights
                // lights.restorePointGlobalStrength(maxIgnoredPointLight);
            }
        } // for each instance
    } // for each renderable

    if (!submitCount) {
        bgfx::touch(mm.mainView);
    }

    lights.postDraw();

    printc(ShowRenderDbgTick, "-------------------------------------------------------------ENDING RENDER FRAME\n");
}

void RenderSystem::shutdown() {
    for (auto node : pool) {
        mm.memMan.request({.ptr=node->ptr, .size=0});
    }
    mm.memMan.request({.ptr=pool, .size=0});

    bgfx::destroy(gltfProgram);
    bgfx::destroy(unlitProgram);
    for (auto & t : loadingThreads) {
        t.second.join();
    }
    loadingThreads.clear();
    samplers.shutdown();
    lights.shutdown();
    fog.shutdown();
    colors.shutdown();
    whiteTexture.destroy();

    bgfx::destroy(materialBaseColor);
    bgfx::destroy(materialPBRValues);
    bgfx::destroy(normModel);
}


// 
// RENDERABLE LIFECYCLE
//

Renderable * RenderSystem::create(bgfx::ProgramHandle program, char const * key) {
    if (strcmp(key, "") == 0) {
        printc(ShowRenderDbg, "Key cannot be blank.\n");
        return nullptr;
    }

    if (keyExists(key)) {
        printc(ShowRenderDbg, "WARNING: did not create renderable. Key already in use.\n");
        return nullptr;
    }

    // create renderable and map to location
    Renderable * rptr = mm.memMan.create<Renderable>();
    CharKeys::Status status = pool->insert(key, rptr);
    if (status != CharKeys::SUCCESS) {
        printc(ShowRenderDbg, "Problem adding renderable pointer to pool.\n");
        return nullptr;
    }
    Renderable & r = *rptr;

    printc(ShowRenderDbg, "CREATING %s AT %p\n", key, rptr);
    r.program = program;
    fixstrcpy<CharKeys::KEY_MAX>(r.key, key);
    r.resetInstances(1);

    showMoreStatus("(AFTER CREATE)");
    return &r;
}

Renderable * RenderSystem::createFromGLTF(char const * filename, char const * key) { 
    printc(ShowRenderDbg, "Attempting to load for key(%s) : %s.\n", key, relPath(filename));
    // if this key is already loading somewhere, kill this load request
    if (!isKeySafeToDrawOrLoad(key)) {
        printc(ShowRenderDbg, "at(%s)->isSafeToDraw = %d", key, at(key)->isSafeToDraw);
        printc(ShowRenderDbg, "WARNING: did not initiate load. Renderable with this key is currently loading.\n");
        return nullptr;
    }

    // create renderable
    Renderable * r;
    if (keyExists(key)) {
        r = at(key);
        reset(key);
    }
    else {
        r = create(gltfProgram, key);
        if (!r) {
            printc(ShowRenderDbg, "WARNING Renderable not created.\n");
            return nullptr;
        }
    }

    r->isSafeToDraw = false;

    // !!!
    // spawns thread by emplacing in std::unordered_map<Renderable *, std::thread>
    // will be joined by RenderSystem::draw when load complete detected
    loadingThreads.emplace(r, [=]{
        gltfLoader.load(filename, *r);
    });

    return r;
}

Renderable * RenderSystem::at(char const * key) {
    return (Renderable *)pool->ptrForKey(key);
}

bool RenderSystem::destroy(char const * key) {
    printc(ShowRenderDbg, "Destroying renderable(%s).\n", key);
    if (!pool->hasKey(key)) {
        printc(ShowRenderDbg, "WARNING: did not destroy renderable. Key not found.");
        return false;
    }

    showMoreStatus("(BEFORE DESTROY)");

    Renderable * r = at(key);

    // reset the renderable object slot
    // frees a bunch of stuff so don't foreget it!
    reset(key);
    r->resetInstances(0);

    // delete the renderable in the pool
    pool->remove(key);
    mm.memMan.request({.ptr=r, .size=0});

    showMoreStatus("(AFTER DESTROY)");

    return true;
}

void RenderSystem::reset(char const * key) {
    auto & r = *at(key);

    // foreach mesh
    auto handleM = [](Mesh & m) {
        for (auto vbuf : m.vbufs) {
            bgfx::destroy(vbuf);
        }
        if (isValid(m.ibuf)) {
            bgfx::destroy(m.ibuf);
        }
    };
    for (auto & m : r.meshes) { handleM(m); }
    for (auto & m : r.meshesWithAlpha) { handleM(m); }
    r.meshes.clear();
    r.meshesWithAlpha.clear();

    if (r.buffer) {
        free(r.buffer);
        r.buffer = nullptr;
        r.bufferSize = 0;
    }

    for (auto & image : r.images) {
        if (image.data) {
            free(image.data);
            image.data = nullptr;
            image.dataSize = 0;
        }
    }
    r.images.clear();

    for (auto & texture : r.textures) {
        bgfx::destroy(texture);
    }
    r.textures.clear();

    r.materials.clear();
}

// 
// RENDERABLE UTILS
//

bool RenderSystem::keyExists(char const * key) {
    return pool->hasKey(key);
}

bool RenderSystem::isKeySafeToDrawOrLoad(char const * key) {
    // key doesn't exist. safe to load.
    if (!keyExists(key)) return true;
    // key exists, find renderable and check property.
    return at(key)->isSafeToDraw;
}

bool RenderSystem::isRenderableLoading(Renderable const & r, Renderable::LoadState & foundLoadState) {
    // bail as early as possible
    if (loadingThreads.size() == 0 || 
        !keyExists(r.key) || 
        loadingThreads.count(at(r.key)) == 0
        ) {
        return false;
    }
    gltfLoader.loadingMutex.lock();
    foundLoadState = r.loadState;
    gltfLoader.loadingMutex.unlock();
    return (foundLoadState == Renderable::LoadState::Loading);
}

void RenderSystem::showStatus() {
    printc(ShowRenderDbg, "RENDERABLE COUNT: %zu\n", pool->nNodes());
}

void RenderSystem::showMoreStatus(char const * prefix) {
    if (strcmp(prefix, "")) {
        printc(ShowRenderDbg, "%s ", prefix);
    }
    if constexpr (ShowRenderDbg) {
        char * buf = mm.frameStr(1024);
        pool->printToBuf(buf, 1024);
        printc(ShowRenderDbg, "RENDERABLEPOOL: \n%s\n", buf);
    }
}

size_t RenderSystem::renderableCount() const {
    return pool->nNodes();
}
