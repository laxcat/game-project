#include "RenderSystem.h"
#include <bgfx/platform.h>
#include <bimg/decode.h>
#include <bx/error.h>
#include "../MrManager.h"
#include "../common/modp_b64.h"
#include "../common/string_utils.h"
#include "../render/bgfx_extra.h"
#include "../memory/CharKeys.h"
#include "../memory/File.h"

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
    lights.init();
    fog.init();
    colors.init();
    samplerColor = bgfx::createUniform("s_color", bgfx::UniformType::Sampler);
    samplerNorm  = bgfx::createUniform("s_norm",  bgfx::UniformType::Sampler);
    samplerMetal = bgfx::createUniform("s_metal", bgfx::UniformType::Sampler);
    materialBaseColor = bgfx::createUniform("u_materialBaseColor", bgfx::UniformType::Vec4);
    materialPBRValues = bgfx::createUniform("u_materialPBRValues", bgfx::UniformType::Vec4);
    normModel = bgfx::createUniform("u_normModel", bgfx::UniformType::Mat3);

    renderList = mm.memMan.createCharKeys(8);
}

void RenderSystem::draw() {
    printc(ShowRenderDbgTick, "STARTING RENDER FRAME-----------------------------------------------------------\n");

    lights.preDraw();
    mm.camera->preDraw();
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

        if (settings.user.drawSceneAABB) {
            if (!Gobj::isValid(gobj->bounds.renderHandleIndex)) {
                static uint16_t const data[24] = {
                    // bottom
                    0, 1, 1, 2, 2, 3, 3, 0,
                    // sides
                    0, 4, 1, 5, 2, 6, 3, 7,
                    // top
                    4, 5, 5, 6, 6, 7, 7, 4
                };
                auto indexRef = bgfx::makeRef(data, 24 * sizeof(uint16_t));
                gobj->bounds.renderHandleIndex = bgfx::createIndexBuffer(indexRef).idx;
            }
            if (!Gobj::isValid(gobj->bounds.renderHandleVertex)) {
                bgfx::VertexLayout layout;
                layout.begin();
                layout.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float);
                layout.end();
                static glm::vec3 data[8];
                gobj->bounds.fillCubePoints(data);
                auto ref = bgfx::makeRef(data, sizeof(float) * 3 * 8);
                gobj->bounds.renderHandleVertex = bgfx::createVertexBuffer(ref, layout).idx;
            }

            bgfx::setVertexBuffer(0, bgfx::VertexBufferHandle{gobj->bounds.renderHandleVertex});
            bgfx::setIndexBuffer(bgfx::IndexBufferHandle{gobj->bounds.renderHandleIndex});

            bgfx::setUniform(materialBaseColor, settings.user.aabbColor);

            // set transform
            static glm::mat4 transform{1.f};
            bgfx::setTransform(&transform);

            // set modified state
            uint64_t state = settings.state;
            state |= BGFX_STATE_PT_LINES;
            bgfx::setState(state);

            // submit
            bgfx::submit(mm.mainView, unlitProgram);
            ++submitCount;
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
        Gobj::Material * mat = prim->material;

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

        state |= bgfxPrimitiveType(prim->mode);

        // require material
        assert(mat                           && "Set minimum material during setup if not in Gobj.");
        assert(mat->baseColorTexture         && "Set minimum material baseColorTexture during setup if not in Gobj.");
        assert(mat->normalTexture            && "Set minimum material normalTexture during setup if not in Gobj.");
        assert(mat->metallicRoughnessTexture && "Set minimum material metallicRoughnessTexture during setup if not in Gobj.");

        // set textures
        bgfx::setTexture(
            TEXTURE_SLOT_COLOR,
            samplerColor,
            bgfx::TextureHandle{mat->baseColorTexture->renderHandle}
        );
        bgfx::setTexture(
            TEXTURE_SLOT_NORM,
            samplerNorm,
            bgfx::TextureHandle{mat->normalTexture->renderHandle}
        );
        bgfx::setTexture(
            TEXTURE_SLOT_METAL,
            samplerMetal,
            bgfx::TextureHandle{mat->metallicRoughnessTexture->renderHandle}
        );
        bgfx::setUniform(materialBaseColor, mat->baseColorFactor);

        if (mat->alphaMode == Gobj::Material::ALPHA_BLEND ||
            mat->baseColorFactor[3] < 1.f) {
            state |= BGFX_STATE_BLEND_ALPHA;
        }

        // glm::vec4 pbrValues{
        //     0.5f, // roughness. 0 smooth, 1 rough
        //     0.0f, // metallic. 0 plastic, 1 metal
        //     0.2f, // specular, additional specular adjustment for non-matalic materials
        //     1.0f  // color intensity
        // };
        glm::vec4 pbrValues{
            mat->roughnessFactor,
            mat->metallicFactor,
            0.2f, // unused
            1.0f, // unused
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
    bgfx::destroy(samplerColor);
    bgfx::destroy(samplerNorm);
    bgfx::destroy(samplerMetal);
    lights.shutdown();
    fog.shutdown();
    colors.shutdown();

    bgfx::destroy(materialBaseColor);
    bgfx::destroy(materialPBRValues);
    bgfx::destroy(normModel);
}

Gobj * RenderSystem::add(char const * key, Gobj * gobj) {
    if (renderList->isFull()) {
        fprintf(stderr, "Could not add Gobj (%p) to %s\n", gobj, key);
        return nullptr;
    }
    gobj = addMinReqMat(gobj);
    renderList->insert(key, gobj);
    postAdd(gobj);

    return gobj;
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
    newGobj = addMinReqMat(newGobj);
    renderList->update(key, newGobj);
    postAdd(newGobj);
    // return removed gobj
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

void RenderSystem::postAdd(Gobj * gobj) {
    addHandles(gobj);
    mm.camera->zoomTo(gobj);
}

Gobj * RenderSystem::addMinReqMat(Gobj * gobj) {
    if (!needsMinReqMat(gobj)) {
        return gobj;
    }

    gobj = mm.memMan.updateGobj(gobj, countsForMinReqMat(gobj));

    static char const * white2x2x3PNG =
    "data:image/png;base64,"
    // 2x2 PNG, filled with 0xffffff (r,g,b) for every 3-byte pixel:
    "iVBORw0KGgoAAAANSUhEUgAAAAIAAAACCAIAAAD91JpzAAAAE0lEQVR4XmP8//8/AwMDEwMYAA"
    "AkBgMBTJCz6gAAAABJRU5ErkJggg==";

    static char const * metalRoughPNG =
    "data:image/png;base64,"
    //          (g) roughness---v v---metalic (b)
    // 2x2 PNG, filled with 0xffff00ff (r,g,b,a) for every 4-byte pixel:
    "iVBORw0KGgoAAAANSUhEUgAAAAIAAAACCAYAAABytg0kAAAAE0lEQVR4XmP8/5/hPwMQMDFAAQ"
    "AvBwMBP8AQXgAAAABJRU5ErkJggg==";

    Gobj::Image * normColorImg = gobj->images + gobj->counts.images - 2;
    normColorImg->uri = white2x2x3PNG;
    Gobj::Image * metalRoughImg = gobj->images + gobj->counts.images - 1;
    metalRoughImg->uri = metalRoughPNG;

    Gobj::Texture * normColorTex = gobj->textures + gobj->counts.textures - 2;
    normColorTex->source = normColorImg;
    Gobj::Texture * metalRoughTex = gobj->textures + gobj->counts.textures - 1;
    metalRoughTex->source = metalRoughImg;

    Gobj::Material * newMat = gobj->materials + gobj->counts.materials - 1;
    newMat->baseColorTexture = normColorTex;
    newMat->normalTexture = normColorTex;
    newMat->metallicRoughnessTexture = metalRoughTex;

    for (uint16_t m = 0; m < gobj->counts.meshes; ++m) {
        Gobj::Mesh * mesh = gobj->meshes + m;
        for (uint16_t p = 0; p < mesh->nPrimitives; ++p) {
            Gobj::MeshPrimitive * prim = mesh->primitives + p;
            if (prim->material == nullptr) {
                printl("adding default material to mesh %u (%s) primative %u", m, mesh->name, p);
                prim->material = newMat;
            }
            else {
                if (prim->material->baseColorTexture == nullptr) {
                    printl("adding default norm/color texture to baseColorTexture of mesh %u (%s) primative %u", m, mesh->name, p);
                    prim->material->baseColorTexture = normColorTex;
                }
                if (prim->material->normalTexture == nullptr) {
                    printl("adding default norm/color texture to normalTexture of mesh %u (%s) primative %u", m, mesh->name, p);
                    prim->material->normalTexture = normColorTex;
                }
                if (prim->material->metallicRoughnessTexture == nullptr) {
                    printl("adding default metal/rough texture to metallicRoughnessTexture of mesh %u (%s) primative %u", m, mesh->name, p);
                    prim->material->metallicRoughnessTexture = metalRoughTex;
                }
            }
        }
    }

    return gobj;
}

bool RenderSystem::needsMinReqMat(Gobj * gobj) {
    if (gobj->counts.materials < 1 ||
        gobj->counts.textures < 1
    ) {
        return true;
    }
    for (uint16_t i = 0; i < gobj->counts.materials; ++i) {
        Gobj::Material * mat = gobj->materials + i;
        if (mat->baseColorTexture == nullptr ||
            mat->normalTexture == nullptr ||
            mat->metallicRoughnessTexture == nullptr
        ) {
            return true;
        }
    }
    return false;
}

Gobj::Counts RenderSystem::countsForMinReqMat(Gobj * gobj) {
    return {
        .materials = 1,
        .textures = 2,
        .images = 2,
    };
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
        // skip if texture already created
        if (Gobj::isValid(tex->renderHandle)) {
            continue;
        }
        mm.createWorker([this, tex, gobj]{
            bimg::ImageContainer * imgc = decodeImage(tex->source, gobj->loadedDirName);
            if (imgc == nullptr) {
                return;
            }
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

bimg::ImageContainer * RenderSystem::decodeImage(Gobj::Image * img, char const * loadedDirName) {
    if (!img) return nullptr;
    if (img->decoded) return (bimg::ImageContainer *)img->decoded;

    byte_t * data = nullptr;
    uint32_t dataSize = 0;
    // encoded image data is in gobj buffer
    if (img->bufferView) {
        Gobj::BufferView & bv = *img->bufferView;
        data = bv.buffer->data + bv.byteOffset;
        dataSize = bv.byteLength;
    }
    // encoded image data is in external file
    else if (img->uri) {
        // data encoded image
        if (strlen(img->uri) >= 8 && strEqu(img->uri, "data:ima", 8)) {
            static char const * isDataStr = "data:image/png;base64,";
            static size_t isDataLen = strlen(isDataStr);
            assert(strEqu(img->uri, isDataStr, isDataLen) && "Unexpected data string header.");

            size_t b64StrLen = strlen(img->uri) - isDataLen;
            dataSize = modp_b64_decode_len(b64StrLen);
            data = (byte_t *)mm.memMan.request({.size=dataSize, .high=true, .lifetime=0});
            modp_b64_decode((char *)data, (char *)img->uri + isDataLen, b64StrLen);

            // fprintf(stderr, "data-encoded image URIs not supported.\n");
            // return nullptr;
        }
        // image file
        else {
            char const * fullPath = mm.frameFormatStr("%s%s", loadedDirName, img->uri);
            File * f = mm.memMan.createFileHandle(fullPath, true, {.high=true, .lifetime=0});
            data = f->data();
            dataSize = f->size();
        }
    }
    else {
        fprintf(stderr, "No bufferView or URI set on Gobj::Image (%p).\n", img);
        return nullptr;
    }

    // printl("Loading image at %p for %u bytes", data, bv.byteLength);
    bx::Error err;
    bimg::ImageContainer * imgc = imageParse(
        &bxAllocator,
        data,
        dataSize,
        bimg::TextureFormat::Count,
        &err
    );
    if (imgc && err.isOk()) {
        printl("loaded image %s, data at: %p, w: %u, h: %u, d: %u",
            img->name, imgc->m_data, imgc->m_width, imgc->m_height, imgc->m_depth);
    }
    else {
        fprintf(stderr, "error loading image: %s (%p, %u)\n", err.getMessage().getPtr(), data, dataSize);
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

