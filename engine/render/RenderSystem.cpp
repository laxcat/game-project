#include "RenderSystem.h"
#include <bgfx/platform.h>
#include "../MrManager.h"
#include "../common/string_utils.h"
#include "../render/bgfx_extra.h"
#include "../memory/CharKeys.h"

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

    renderList = mm.memMan.createCharKeys(8);

    byte_t data[] = {
        255,255,255,255,
        255,255,255,255,
        255,255,255,255,
        255,255,255,255
    };
    whiteTexture.createImmutable(2, 2, 4, data);
}

// void RenderSystem::draw() {
//     printc(ShowRenderDbgTick, "STARTING RENDER FRAME-----------------------------------------------------------\n");

//     lights.preDraw();
//     mm.camera.preDraw();
//     bgfx::setUniform(fog.handle, (float *)&fog.data);
//     bgfx::setUniform(colors.background.handle, (float *)&colors.background.data);

//     size_t submitCount = 0;

//     // for each renderable
//     for (auto node : pool) {
//         auto r = (Renderable *)node->ptr;

//         // skip if ready to draw but not active
//         if (!r->hasActiveInstance() && r->isSafeToDraw) {
//             continue;
//         }

//         printc(ShowRenderDbgTick,
//             "----------------------------------------\n"
//             "RENDERABLE %s (%p)\n"
//             "----------------------------------------\n",
//             r->key, r->key
//         );

//         // is loading
//         if (!r->isSafeToDraw) {
//             // if renderable is loading, skip draw
//             auto foundLoadState = Renderable::LoadState::NotLoading;
//             if (isRenderableLoading(r, foundLoadState)) {
//                 continue;
//             }
//             // done loading, finalize renderable, join threads
//             else if (foundLoadState == Renderable::LoadState::WaitingToFinalize) {
//                 gltfLoader.loadingMutex.lock();
//                 r->loadState = Renderable::LoadState::NotLoading;
//                 gltfLoader.loadingMutex.unlock();
//                 r->isSafeToDraw = true;

//                 loadingThreads.at(r).join();
//                 loadingThreads.erase(r);
//                 printc(ShowRenderDbg, "Finished loading renderable %p\n", r);
//             }
//             else if (foundLoadState == Renderable::LoadState::FailedToLoad) {
//                 printc(ShowRenderDbg, "FAILING TO LOAD\n");
//                 loadingThreads.at(r).join();
//                 loadingThreads.erase(r);
//                 reset(r);
//                 continue;
//             }
//         }
//         // !!!
//         // IMPORTANT FOR THREAD SAFETY
//         // should not make it to this point if renderable is still loading
//         // !!!
//         // r.loadState is guanteed Renderable::LoadState::NotLoading (or FailedToLoad) at this point
//         // !!!

//         // draw model instances... not using GPU intancing (but it could be optimized to do so!)
//         for (size_t i = 0, e = r->instances.size(); i < e; ++i) {
//             auto const & instance = r->instances[i];
//             // skip if not active
//             if (!instance.active) continue;

//             printc(ShowRenderDbgTick,
//                 "--------------------\n"
//                 "instance %zu\n"
//                 "--------------------\n",
//                 i
//             );

//             // draw meshes
//             for (auto const & mesh : r->meshes) {

//                 printc(ShowRenderDbgTick,
//                     "----------\n"
//                     "mesh %p\n"
//                     "----------\n",
//                     &mesh
//                 );

//                 if constexpr (ShowRenderDbg) {
//                     if (mesh.renderableKey == nullptr || strcmp(mesh.renderableKey, r->key) != 0) {
//                         print("WARNING! Key not set on mesh in %s\n", r->key);
//                     }
//                 }

//                 // set buffers
//                 if (isValid(mesh.dynvbuf)) {
//                     bgfx::setVertexBuffer(0, mesh.dynvbuf);
//                 }
//                 else {
//                     for (size_t i = 0; i < mesh.vbufs.size(); ++i) {
//                         bgfx::setVertexBuffer(i, mesh.vbufs[i]);
//                     }
//                 }
//                 bgfx::setIndexBuffer(mesh.ibuf);
//                 bgfx::setState(mm.rendSys.settings.state);

//                 // set textures
//                 if (mesh.images.color >= 0) {
//                     bgfx::setTexture(0, samplers.color, r->textures[mesh.images.color]);
//                 }
//                 else {
//                     bgfx::setTexture(0, samplers.color, whiteTexture.handle);
//                 }
//                 auto & material = mesh.getMaterial();
//                 glm::vec4 color = material.baseColor;
//                 if (instance.overrideColor != glm::vec4{1.f, 1.f, 1.f, 1.f}) {
//                     color = instance.overrideColor;
//                 }
//                 bgfx::setUniform(materialBaseColor, &color);
//                 bgfx::setUniform(materialPBRValues, &material.pbrValues);

//                 // get this meshes's final model
//                 auto model = instance.model * r->adjRotModel * mesh.model;
//                 bgfx::setTransform((float *)&model);

//                 // make a reduced version of the rotation for the shader normals
//                 auto nm = glm::transpose(glm::inverse(glm::mat3{model}));
//                 bgfx::setUniform(normModel, (float *)&nm);

//                 // disable ignored lights
//                 lights.ignorePointLights(instance.ignorePointLights);

//                 // submit
//                 bgfx::submit(mm.mainView, r->program);
//                 ++submitCount;

//                 // restor any ignored lights
//                 // lights.restorePointGlobalStrength(maxIgnoredPointLight);
//             }
//         } // for each instance
//     } // for each renderable

//     if (!submitCount) {
//         bgfx::touch(mm.mainView);
//     }

//     lights.postDraw();

//     printc(ShowRenderDbgTick, "-------------------------------------------------------------ENDING RENDER FRAME\n");
// }

void RenderSystem::draw() {
    printc(ShowRenderDbgTick, "STARTING RENDER FRAME-----------------------------------------------------------\n");

    lights.preDraw();
    mm.camera.preDraw();
    bgfx::setUniform(fog.handle, (float *)&fog.data);
    bgfx::setUniform(colors.background.handle, (float *)&colors.background.data);

    size_t submitCount = 0;

    // for each renderable
    for (auto node : renderList) {
        // auto r = (Renderable *)node->ptr;
        auto g = (Gobj *)node->ptr;

        printc(ShowRenderDbgTick,
            "----------------------------------------\n"
            "RENDERABLE %s (%p)\n"
            "----------------------------------------\n",
            // r->key, r->key
            node->key, g
        );

        // each mesh
        for (uint16_t meshIndex = 0; meshIndex < g->counts.meshes; ++meshIndex) {
            submitCount += drawMesh(g->meshes[meshIndex]);
        }
    }

    if (!submitCount) {
        bgfx::touch(mm.mainView);
    }

    lights.postDraw();

    printc(ShowRenderDbgTick, "-------------------------------------------------------------ENDING RENDER FRAME\n");
}

uint16_t RenderSystem::drawMesh(Gobj::Mesh const & mesh) {
    uint16_t submitCount = 0;

    printc(ShowRenderDbgTick,
        "----------\n"
        "mesh %p (%s), %u primitives\n"
        "----------\n",
        &mesh, mesh.name, mesh.nPrimitives
    );

    // each primitive
    for (int primIndex = 0; primIndex < mesh.nPrimitives; ++primIndex) {
        Gobj::MeshPrimitive & prim = mesh.primitives[primIndex];
        printc(ShowRenderDbgTick,
            "-----\n"
            "mesh primitive %p\n"
            "-----\n",
            &prim
        );

        // set buffers
        for (int attrIndex = 0; attrIndex < prim.nAttributes; ++attrIndex) {
            Gobj::Accessor & acc = *prim.attributes[attrIndex].accessor;
            bgfx::setVertexBuffer(attrIndex, bgfx::VertexBufferHandle{acc.renderHandle});
        }
        bgfx::setIndexBuffer(bgfx::IndexBufferHandle{prim.indices->renderHandle});
        bgfx::setState(settings.state);

        // set textures
        bgfx::setTexture(0, samplers.color, whiteTexture.handle);
        Gobj::Material & material = *prim.material;
        bgfx::setUniform(materialBaseColor, &material.baseColorFactor);
        glm::vec4 pbrValues{
            0.5f, // roughness. 0 smooth, 1 rough
            0.0f, // metallic. 0 plastic, 1 metal
            0.2f, // specular, additional specular adjustment for non-matalic materials
            1.0f  // color intensity
        };
        bgfx::setUniform(materialPBRValues, &pbrValues);

        // set model
        float model[16] = {
            1.f, 0.f, 0.f, 0.f,
            0.f, 1.f, 0.f, 0.f,
            0.f, 0.f, 1.f, 0.f,
            0.f, 0.f, 0.f, 1.f
        };
        bgfx::setTransform(model);

        // make a reduced version of the rotation for the shader normals
        auto nm = glm::transpose(glm::inverse(glm::mat3{*(glm::mat4 *)model}));
        bgfx::setUniform(normModel, (float *)&nm);

        // submit
        bgfx::submit(mm.mainView, gltfProgram);
        ++submitCount;
    } // for each primitive

    return submitCount;
}

void RenderSystem::shutdown() {
    // destroy
    for (auto node : renderList) {
        removeHandles((Gobj *)node->ptr);
    }
    // no need to ever dealloc memMan objects TODO: is this still true?
    // mm.memMan.request({.ptr=renderList, .size=0});

    bgfx::destroy(gltfProgram);
    bgfx::destroy(unlitProgram);
    // for (auto & t : loadingThreads) {
    //     t.second.join();
    // }
    // loadingThreads.clear();
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

// Renderable * RenderSystem::create(bgfx::ProgramHandle program, char const * key) {
//     assert(false);
//     if (strcmp(key, "") == 0) {
//         printc(ShowRenderDbg, "Key cannot be blank.\n");
//         return nullptr;
//     }

//     if (keyExists(key)) {
//         printc(ShowRenderDbg, "WARNING: did not create renderable. Key already in use.\n");
//         return nullptr;
//     }

//     // create renderable and map to location
//     Renderable * rptr = mm.memMan.create<Renderable>();
//     CharKeys::Status status = pool->insert(key, rptr);
//     if (status != CharKeys::SUCCESS) {
//         printc(ShowRenderDbg, "Problem adding renderable pointer to pool.\n");
//         return nullptr;
//     }
//     Renderable & r = *rptr;

//     printc(ShowRenderDbg, "CREATING %s AT %p\n", key, rptr);
//     r.program = program;
//     r.setKey(key);
//     r.resetInstances(1);

//     showMoreStatus("(AFTER CREATE)");
//     return &r;
// }

// Renderable * RenderSystem::createFromGLTF(char const * filename, char const * key) {
//     printc(ShowRenderDbg, "Attempting to load for key(%s) : %s.\n", key, filename);
//     // if this key is already loading somewhere, kill this load request
//     if (!isKeySafeToDrawOrLoad(key)) {
//         printc(ShowRenderDbg, "at(%s)->isSafeToDraw = %d", key, at(key)->isSafeToDraw);
//         printc(ShowRenderDbg, "WARNING: did not initiate load. Renderable with this key is currently loading.\n");
//         return nullptr;
//     }

//     // create renderable
//     Renderable * r;
//     if (keyExists(key)) {
//         r = at(key);
//         reset(r);
//     }
//     else {
//         r = create(gltfProgram, key);
//         if (!r) {
//             printc(ShowRenderDbg, "WARNING Renderable not created.\n");
//             return nullptr;
//         }
//     }

//     #if DEV_INTERFACE
//     printl("setting filename %s", filename);
//     r->path = filename;
//     printl("set filename %s, %s", r->path.full, r->path.filename);
//     #endif // DEV_INTERFACE
//     r->isSafeToDraw = false;

//     // !!!
//     // spawns thread by emplacing in std::unordered_map<Renderable *, std::thread>
//     // will be joined by RenderSystem::draw when load complete detected
//     loadingThreads.emplace(r, [=]{
//         gltfLoader.load(r);
//     });

//     return r;
// }

// Renderable * RenderSystem::at(char const * key) {
//     assert(false);
//     return (Renderable *)pool->ptrForKey(key);
// }

void RenderSystem::add(char const * key, Gobj * g) {
    if (renderList->isFull()) return;
    renderList->insert(key, g);
    addHandles(g);
}

void RenderSystem::remove(char const * key) {
    auto g = (Gobj *)renderList->ptrForKey(key);
    if (!g) {
        fprintf(stderr, "WARNING: did not remove Gobj at key:%s. Could not find key.", key);
        return;
    }
    removeHandles(g);
    renderList->remove(key);
}

Gobj * RenderSystem::update(char const * key, Gobj * newGobj) {
    auto oldGobj = (Gobj *)renderList->ptrForKey(key);

    if (!oldGobj) {
        fprintf(stderr, "WARNING: did not update Gobj at key:%s. Could not find key.", key);
        return nullptr;
    }
    removeHandles(oldGobj);
    renderList->update(key, newGobj);
    addHandles(newGobj);
    return oldGobj;
}

// bool RenderSystem::destroy(char const * key) {
//     assert(false);
//     printc(ShowRenderDbg, "Destroying renderable(%s).\n", key);
//     if (!pool->hasKey(key)) {
//         printc(ShowRenderDbg, "WARNING: did not destroy renderable. Key not found.");
//         return false;
//     }

//     showMoreStatus("(BEFORE DESTROY)");

//     Renderable * r = at(key);

//     // reset the renderable object slot
//     // frees a bunch of stuff so don't foreget it!
//     reset(r);
//     r->resetInstances(0);

//     // delete the renderable in the pool
//     pool->remove(key);
//     mm.memMan.request({.ptr=r, .size=0});

//     showMoreStatus("(AFTER DESTROY)");

//     return true;
// }

// void RenderSystem::reset(Renderable * r) {
//     // foreach mesh
//     auto handleM = [](Mesh & m) {
//         for (auto vbuf : m.vbufs) {
//             bgfx::destroy(vbuf);
//         }
//         if (isValid(m.ibuf)) {
//             bgfx::destroy(m.ibuf);
//         }
//     };
//     for (auto & m : r->meshes) { handleM(m); }
//     for (auto & m : r->meshesWithAlpha) { handleM(m); }
//     r->meshes.clear();
//     r->meshesWithAlpha.clear();

//     if (r->buffer) {
//         free(r->buffer);
//         r->buffer = nullptr;
//         r->bufferSize = 0;
//     }

//     for (auto & image : r->images) {
//         if (image.data) {
//             free(image.data);
//             image.data = nullptr;
//             image.dataSize = 0;
//         }
//     }
//     r->images.clear();

//     for (auto & texture : r->textures) {
//         bgfx::destroy(texture);
//     }
//     r->textures.clear();

//     r->materials.clear();

//     r->loadState = Renderable::LoadState::NotLoading;
//     r->isSafeToDraw = true;

//     r->path = "";
// }

// 
// RENDERABLE UTILS
//

bool RenderSystem::keyExists(char const * key) {
    return renderList->hasKey(key);
}

// bool RenderSystem::isKeySafeToDrawOrLoad(char const * key) {
//     // key doesn't exist. safe to load.
//     if (!keyExists(key)) return true;
//     // key exists, find renderable and check property.
//     return at(key)->isSafeToDraw;
// }

// bool RenderSystem::isRenderableLoading(Renderable const * r, Renderable::LoadState & foundLoadState) {
//     // bail as early as possible
//     if (loadingThreads.size() == 0 ||
//         !keyExists(r->key) ||
//         loadingThreads.count(r) == 0
//         ) {
//         return false;
//     }
//     gltfLoader.loadingMutex.lock();
//     foundLoadState = r->loadState;
//     gltfLoader.loadingMutex.unlock();
//     return (foundLoadState == Renderable::LoadState::Loading);
// }

#if DEBUG || DEV_INTERFACE

void RenderSystem::showStatus() {
    printc(ShowRenderDbg, "RENDERABLE COUNT: %zu\n", renderList->nNodes());
}

void RenderSystem::showMoreStatus(char const * prefix) {
    if (strcmp(prefix, "")) {
        printc(ShowRenderDbg, "%s ", prefix);
    }
    if constexpr (ShowRenderDbg && DEBUG) {
        char * buf = mm.frameStr(1024);
        renderList->printToBuf(buf, 1024);
        printc(ShowRenderDbg, "RENDERABLEPOOL: \n%s\n", buf);
    }
}

#endif // DEBUG || DEV_INTERFACE

size_t RenderSystem::renderableCount() const {
    return renderList->nNodes();
}

void RenderSystem::addHandles(Gobj * g) {
    // printl("setup gobj render handles for bgfx");
    for (uint16_t meshIndex = 0; meshIndex < g->counts.meshes; ++meshIndex) {
        auto & mesh = g->meshes[meshIndex];
        // printl("mesh %p (%s)", &mesh, mesh.name);

        for (uint16_t primIndex = 0; primIndex < mesh.nPrimitives; ++primIndex) {
            Gobj::MeshPrimitive & prim = mesh.primitives[primIndex];

            // setup vbuffers
            for (size_t attrIndex = 0; attrIndex < prim.nAttributes; ++attrIndex) {
                Gobj::MeshAttribute & attr = prim.attributes[attrIndex];

                // printl("attr %d %p", attr.type, attr.accessor);

                Gobj::Accessor & acc = *attr.accessor;
                Gobj::BufferView & bv = *acc.bufferView;
                assert(acc.componentType == Gobj::Accessor::COMPTYPE_FLOAT &&
                    "Unexpected accessor component type for vbuffer.");

                bgfx::VertexLayout layout;
                layout.begin();
                layout.add(
                    (bgfx::Attrib::Enum)(int)attr.type,
                    acc.componentCount(),
                    bgfxAttribTypeFromAccessorComponentType(acc.componentType)
                );
                layout.skip(bv.byteStride - acc.byteSize());
                layout.end();

                auto data = bv.buffer->data + acc.byteOffset + bv.byteOffset;
                auto ref = bgfx::makeRef(data, bv.byteLength);

                acc.renderHandle = bgfx::createVertexBuffer(ref, layout).idx;
                // printl("creating vbuffer handle %u", acc.renderHandle);
            }

            // setup ibuffer
            Gobj::Accessor & iacc = *prim.indices;
            Gobj::BufferView & ibv = *iacc.bufferView;
            assert(iacc.componentType == Gobj::Accessor::COMPTYPE_UNSIGNED_SHORT &&
                "Unexpected accessor component type for ibuffer.");
            auto data = ibv.buffer->data + iacc.byteOffset + ibv.byteOffset;
            auto indexRef = bgfx::makeRef(data, iacc.count * sizeof(uint16_t));
            iacc.renderHandle = bgfx::createIndexBuffer(indexRef).idx;
            // printl("creating ibuffer handle %u", iacc.renderHandle);

            // printl("primative %p, index accessor name/type %s/%d, data %p, render handle: %u",
            //     &prim, iacc.name, iacc.componentType, data, iacc.renderHandle);
        }
    }
}

void RenderSystem::removeHandles(Gobj * g) {
    for (uint16_t i = 0; i < g->counts.accessors; ++i) {
        auto & acc = g->accessors[i];
        if (acc.componentType == Gobj::Accessor::COMPTYPE_UNSIGNED_SHORT &&
            isValid(bgfx::IndexBufferHandle{acc.renderHandle}))
        {
            bgfx::destroy(bgfx::IndexBufferHandle{acc.renderHandle});
        }
        else if (acc.componentType == Gobj::Accessor::COMPTYPE_FLOAT &&
            isValid(bgfx::VertexBufferHandle{acc.renderHandle}))
        {
            bgfx::destroy(bgfx::VertexBufferHandle{acc.renderHandle});
        }
    }
}
