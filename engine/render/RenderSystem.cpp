#include "RenderSystem.h"
#include "../MrManager.h"
#include "../common/bgfx_extra.h"
#include "../common/string_utils.h"

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

static bool constexpr ShowRenderDbg = false;
static bool constexpr ShowRenderDbgTick = ShowRenderDbg && false;

// 
// MANAGER LIFECYCLE
//

void RenderSystem::init() {
    // ensure single thread rendering
    bgfx::renderFrame();

    settings.init(mm.window, mm.windowSize, mm.setup.forceOpenGL);
    if (!bgfx::init(settings.bgfxInit))
        return;

    gltfProgram = createBGFXProgram(vs_gltf_bin, vs_gltf_bin_len, fs_gltf_bin, fs_gltf_bin_len);
    unlitProgram = createBGFXProgram(vs_unlit_bin, vs_unlit_bin_len, fs_unlit_bin, fs_unlit_bin_len);
    samplers.init();
    lights.init();
    fog.init();
    colors.init();
    materialBaseColor = bgfx::createUniform("u_materialBaseColor", bgfx::UniformType::Vec4);
    materialPBRValues  = bgfx::createUniform("u_materialPBRValues",  bgfx::UniformType::Vec4);
    normModel = bgfx::createUniform("u_normModel", bgfx::UniformType::Mat3);

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

    auto rit = renderablePool.begin();
    while (rit != renderablePool.end()) {
        auto & r = *rit;

        // skip if ready to draw but not active
        if (!r.hasActiveInstance() && r.isSafeToDraw) {
            ++rit;
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
                ++rit;
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
                printc(ShowRenderDbg, "FALING TO LOAD");
                loadingThreads.at(&r).join();
                loadingThreads.erase(&r);
                rit = destroy(r.key);
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
        }

        ++rit;
    }

    if (!submitCount) {
        bgfx::touch(mm.mainView);
    }

    lights.postDraw();

    printc(ShowRenderDbgTick, "-------------------------------------------------------------ENDING RENDER FRAME\n");
}

void RenderSystem::shutdown() {
    auto it = renderablePool.begin();
    while (it != renderablePool.end()) {
        it = destroy(it->key);
    }
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

    auto rit = renderablePool.create(key);
    Renderable & renderable = *rit;
    size_t index = rit - renderablePool.begin();
    printc(ShowRenderDbg, "CREATING AT %zu (%s, %p)\n", index, key, &renderable);
    renderable.program = program;
    renderable.key[0] = '\0', strncat(renderable.key, key, 31);
    renderable.resetInstances(1);

    showMoreStatus("(AFTER CREATE)");
    return &renderable;
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
    return &renderablePool.at(key);
}

RenderableIterator RenderSystem::destroy(char const * key) {
    printc(ShowRenderDbg, "Destroying renderable(%s, %zu).\n", key);
    if (!renderablePool.contains(key)) {
        printc(ShowRenderDbg, "WARNING: did not destroy renderable. Key not found.");
        return renderablePool.end();
    }

    showMoreStatus("(BEFORE DESTROY)");

    // reset the renderable object slot
    // frees a bunch of stuff so don't foreget it!
    reset(key);
    renderablePool.at(key).resetInstances(0);

    // delete the renderable in the pool
    auto nextIt = renderablePool.erase(key);

    showMoreStatus("(AFTER DESTROY)");

    return nextIt;
}

void RenderSystem::reset(char const * key) {
    auto & r = renderablePool.at(key);

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
    for (auto & texture : r.textures) {
        bgfx::destroy(texture);
    }
    r.materials.clear();
}

// 
// RENDERABLE UTILS
//

bool RenderSystem::keyExists(char const * key) {
    return renderablePool.contains(key);
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
    printc(ShowRenderDbg, "RENDERABLE COUNT: %zu\n", renderablePool.size());
}

void RenderSystem::showMoreStatus(char const * prefix) {
    if (strcmp(prefix, "")) {
        printc(ShowRenderDbg, "%s ", prefix);
    }
    printc(ShowRenderDbg, "RENDERABLEPOOL: %s\n", renderablePool.toString().c_str());
}
