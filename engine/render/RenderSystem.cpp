#include "RenderSystem.h"
#include <bgfx/platform.h>
#include <bimg/decode.h>
#include <bx/error.h>
#include "../MrManager.h"
#include "../common/string_utils.h"
#include "../render/bgfx_extra.h"
#include "../memory/CharKeys.h"

#if FORCE_OPENGL
    #include "../shader/shaders/standard/vs_standard.sc.glsl.bin.h"
    #include "../shader/shaders/standard/fs_standard.sc.glsl.bin.h"
    #include "../shader/shaders/unlit/vs_unlit.sc.glsl.bin.h"
    #include "../shader/shaders/unlit/fs_unlit.sc.glsl.bin.h"
#else
    #include "../shader/shaders/standard/vs_standard.sc.mtl.bin.h"
    #include "../shader/shaders/standard/fs_standard.sc.mtl.bin.h"
    #include "../shader/shaders/unlit/vs_unlit.sc.mtl.bin.h"
    #include "../shader/shaders/unlit/fs_unlit.sc.mtl.bin.h"
#endif

static bool constexpr ShowRenderDbg = true;
static bool constexpr ShowRenderDbgTick = ShowRenderDbg && false;

void RenderSystem::init() {
    // ensure single thread rendering
    bgfx::renderFrame();

    settings.init(mm.window, mm.windowSize, mm.setup.forceOpenGL);
    bxAllocator.memMan = &mm.memMan;
    settings.bgfxInit.allocator = &bxAllocator;
    if (!bgfx::init(settings.bgfxInit))
        return;

    #if FORCE_OPENGL
        standardProgram = CREATE_BGFX_PROGRAM(standard_glsl);
        unlitProgram = CREATE_BGFX_PROGRAM(unlit_glsl);
    #else
        standardProgram = CREATE_BGFX_PROGRAM(standard_mtl);
        unlitProgram = CREATE_BGFX_PROGRAM(unlit_mtl);
    #endif
    samplers.init();
    lights.init();
    fog.init();
    colors.init();
    materialBaseColor = bgfx::createUniform("u_materialBaseColor", bgfx::UniformType::Vec4);
    materialPBRValues = bgfx::createUniform("u_materialPBRValues",  bgfx::UniformType::Vec4);
    normModel = bgfx::createUniform("u_normModel", bgfx::UniformType::Mat3);

    renderList = mm.memMan.createCharKeys(8);

    byte_t temp[] = {
        255,255,255,255,
        255,255,255,255,
        255,255,255,255,
        255,255,255,255
    };
    whiteTexture = bgfx::createTexture2D(
        (uint16_t)2,
        (uint16_t)2,
        false,
        1,
        bgfx::TextureFormat::Enum::RGBA8,
        BGFX_TEXTURE_NONE|BGFX_SAMPLER_NONE,
        bgfx::copy(temp, 16)
    );

}

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
        auto gobj = (Gobj *)node->ptr;

        if (!gobj->isReadyToDraw()) {
            continue;
        }

        printc(ShowRenderDbgTick,
            "----------------------------------------\n"
            "RENDERABLE %s (%p)\n"
            "----------------------------------------\n",
            // r->key, r->key
            node->key, gobj
        );

        // if scene present, draw through node structure
        if (gobj->scene) {
            // each root node
            for (uint16_t nodeIndex = 0; nodeIndex < gobj->scene->nNodes; ++nodeIndex) {
                submitCount += drawNode(gobj, gobj->scene->nodes[nodeIndex]);
            }
        }
        // otherwise draw all meshes
        else {
            // each mesh
            for (uint16_t meshIndex = 0; meshIndex < gobj->counts.meshes; ++meshIndex) {
                submitCount += drawMesh(gobj, gobj->meshes[meshIndex]);
            }
        }
    }

    // printl("submit count for frame %zu: %d", mm.frame, submitCount);

    if (!submitCount) {
        bgfx::touch(mm.mainView);
    }

    lights.postDraw();

    printc(ShowRenderDbgTick, "-------------------------------------------------------------ENDING RENDER FRAME\n");
}

uint16_t RenderSystem::drawNode(Gobj * gobj, Gobj::Node * node, glm::mat4 const & parentTransform) {
    uint16_t submitCount = 0;
    // calc global transform
    glm::mat4 global = parentTransform * node->matrix;
    // draw self if present
    if (node->mesh) {
        submitCount += drawMesh(gobj, *node->mesh, global);
    }
    // draw children
    for (uint16_t nodeIndex = 0; nodeIndex < node->nChildren; ++nodeIndex) {
        submitCount += drawNode(gobj, node->children[nodeIndex], global);
    }
    return submitCount;
}

uint16_t RenderSystem::drawMesh(Gobj * gobj, Gobj::Mesh const & mesh, glm::mat4 const & transform) {
    uint16_t submitCount = 0;

    printc(ShowRenderDbgTick,
        "----------\n"
        "mesh %p (%s), %u primitives\n"
        "----------\n",
        &mesh, mesh.name, mesh.nPrimitives
    );

    // each primitive
    for (int primIndex = 0; primIndex < mesh.nPrimitives; ++primIndex) {
        Gobj::MeshPrimitive * prim = mesh.primitives + primIndex;
        printc(ShowRenderDbgTick,
            "-----\n"
            "mesh primitive %p\n"
            "-----\n",
            prim
        );

        // set buffers
        for (int attrIndex = 0; attrIndex < prim->nAttributes; ++attrIndex) {
            Gobj::Accessor * acc = prim->attributes[attrIndex].accessor;
            bgfx::setVertexBuffer(attrIndex, bgfx::VertexBufferHandle{acc->renderHandle});
        }
        bgfx::setIndexBuffer(bgfx::IndexBufferHandle{prim->indices->renderHandle});

        uint64_t state = settings.state;
        uint32_t stateRGBA = 0;

        // set textures
        if (prim->material) {
            if (prim->material->baseColorTexture) {
                bgfx::setTexture(0, samplers.color, bgfx::TextureHandle{prim->material->baseColorTexture->renderHandle});
            }
            else {
                bgfx::setTexture(0, samplers.color, whiteTexture);
            }
            bgfx::setUniform(materialBaseColor, prim->material->baseColorFactor);

            if (prim->material->alphaMode == Gobj::Material::ALPHA_BLEND) {
                state |= BGFX_STATE_BLEND_ALPHA;
            }
        }
        else {
            bgfx::setTexture(0, samplers.color, whiteTexture);
            static float temp[4] = {0.5f, 0.5f, 0.5f, 1.0f};
            bgfx::setUniform(materialBaseColor, temp);
        }
        glm::vec4 pbrValues{
            0.5f, // roughness. 0 smooth, 1 rough
            0.0f, // metallic. 0 plastic, 1 metal
            0.2f, // specular, additional specular adjustment for non-matalic materials
            1.0f  // color intensity
        };
        bgfx::setUniform(materialPBRValues, &pbrValues);

        // set transform
        bgfx::setTransform(&transform);

        // make a reduced version of the rotation for the shader normals
        auto nm = glm::transpose(glm::inverse(glm::mat3{transform}));
        bgfx::setUniform(normModel, (float *)&nm);

        // set modified state
        bgfx::setState(state, stateRGBA);

        // submit
        bgfx::submit(mm.mainView, standardProgram);
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

    bgfx::destroy(standardProgram);
    bgfx::destroy(unlitProgram);
    // for (auto & t : loadingThreads) {
    //     t.second.join();
    // }
    // loadingThreads.clear();
    samplers.shutdown();
    lights.shutdown();
    fog.shutdown();
    colors.shutdown();
    bgfx::destroy(whiteTexture);

    bgfx::destroy(materialBaseColor);
    bgfx::destroy(materialPBRValues);
    bgfx::destroy(normModel);
}

void RenderSystem::add(char const * key, Gobj * gobj) {
    if (renderList->isFull()) {
        fprintf(stderr, "Could not add Gobj (%p) to %s\n", gobj, key);
        return;
    }
    renderList->insert(key, gobj);
    addHandles(gobj);
}

void RenderSystem::remove(char const * key) {
    auto gobj = (Gobj *)renderList->ptrForKey(key);
    if (!gobj) {
        fprintf(stderr, "WARNING: did not remove Gobj at key:%s. Could not find key.", key);
        return;
    }
    gobj->setStatus(Gobj::STATUS_LOADED);
    removeHandles(gobj);
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


bool RenderSystem::keyExists(char const * key) {
    return renderList->hasKey(key);
}

Gobj * RenderSystem::gobjForKey(char const * key) {
    return (Gobj *)renderList->ptrForKey(key);
}

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

void RenderSystem::addHandles(Gobj * gobj) {
    gobj->setStatus(Gobj::STATUS_DECODING);

    // setup mesh buffers
    for (uint16_t meshIndex = 0; meshIndex < gobj->counts.meshes; ++meshIndex) {
        auto & mesh = gobj->meshes[meshIndex];
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
                // printl("skipping... %u - %u (=%u)", bv.byteStride, acc.byteSize(), (bv.byteStride-acc.byteSize()));
                assert(bv.byteStride && "Byte-stride of 0 makes no sense.");
                layout.skip(bv.byteStride - acc.byteSize());
                layout.end();

                auto data = bv.buffer->data + acc.byteOffset + bv.byteOffset;
                auto ref = bgfx::makeRef(data, bv.byteLength);

                acc.renderHandle = bgfx::createVertexBuffer(ref, layout).idx;

                // attr.print();
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

    // setup textures
    for (uint16_t texIndex = 0; texIndex < gobj->counts.textures; ++texIndex) {
        Gobj::Texture * tex = gobj->textures + texIndex;
        if (tex->renderHandle != UINT16_MAX) {
            continue;
        }
        mm.createWorker([this, tex]{
            bimg::ImageContainer * imgc = decodeImage(tex->source);
            tex->renderHandle = bgfx::createTexture2D(
                (uint16_t)imgc->m_width,
                (uint16_t)imgc->m_height,
                false,
                1,
                (bgfx::TextureFormat::Enum)imgc->m_format,
                BGFX_TEXTURE_NONE|BGFX_SAMPLER_NONE,
                bgfx::makeRef(imgc->m_data, imgc->m_size)
            ).idx;
        },
        gobj // group
        );
    }

    // no workers in group! set status right away
    if (mm.numberOfWorkersInGroup(gobj) == 0) {
        gobj->setStatus(Gobj::STATUS_READY_TO_DRAW);
    }
    // when all in group are done
    else {
        mm.setWorkerGroupOnComplete(gobj, [gobj]{
            gobj->setStatus(Gobj::STATUS_READY_TO_DRAW);
        });
    }
}

bimg::ImageContainer * RenderSystem::decodeImage(Gobj::Image * img) {
    if (!img) return nullptr;
    if (img->decoded) return (bimg::ImageContainer *)img->decoded;

    assert(img->bufferView && "bufferView required");

    Gobj::BufferView & bv = *img->bufferView;
    byte_t * data = bv.buffer->data + bv.byteOffset;

    // printl("Loading image at %p for %u bytes", data, bv.byteLength);
    bx::Error err;
    bimg::ImageContainer * imgc = imageParse(
        &bxAllocator,
        data,
        bv.byteLength,
        bimg::TextureFormat::Count,
        &err
    );
    if (imgc && err.isOk()) {
        printl("loaded image %s, data at: %p, w: %u, h: %u, d: %u",
            img->name, imgc->m_data, imgc->m_width, imgc->m_height, imgc->m_depth);
    }
    else {
        fprintf(stderr, "error loading image: %s (%p, %u)", err.getMessage().getPtr(), data, bv.byteLength);
        return nullptr;
    }
    img->decoded = imgc;
    return imgc;
}


void RenderSystem::removeHandles(Gobj * gobj) {
    gobj->setStatus(Gobj::STATUS_LOADED);

    // remove buffers
    for (uint16_t i = 0; i < gobj->counts.accessors; ++i) {
        auto & acc = gobj->accessors[i];
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

    // remove textures
    for (uint16_t texIndex = 0; texIndex < gobj->counts.textures; ++texIndex) {
        Gobj::Texture & tex = gobj->textures[texIndex];
        if (isValid(bgfx::TextureHandle{tex.renderHandle})) {
            bgfx::destroy(bgfx::TextureHandle{tex.renderHandle});
        }
        if (tex.source && tex.source->decoded) {
            // TODO: research a bit more. this was crashing. not necessary...?
            // either BGFX handles this or was temporary in the first place
            // mm.memMan.request({.ptr=tex.source->decoded, .size=0});
        }
    }
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
//         r = create(standardProgram, key);
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
