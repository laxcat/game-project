#include "RenderSystem.h"
#include <glm/gtc/type_ptr.hpp>
#include "../dev/print.h"
#include "../MrManager.h"


static bool constexpr ShowPrintOutput = true;

void RenderSystem::init() {
    gltfProgram = mm.memSys.loadProgram("vs_tester", "fs_tester");
    colors.init();
}

void RenderSystem::shutdown()  {
    Renderable * r;
    for (size_t i = 0; i < poolSize; ++i) {
        r = at(i);
        r->active = false;
    }
    colors.shutdown();
    bgfx::destroy(gltfProgram);
}

void RenderSystem::draw() {
    // printc(ShouldPrintOutput, "renderable count: %zu\n", renderables.size());
    for (auto i : renderables) {
        auto r = at(i);
        for (auto const & mesh : r->meshes) {
            for (size_t i = 0; i < mesh.dvbufs.size(); ++i) {
                bgfx::update(mesh.dvbufs[i], 0, mesh.vertsRef());
                bgfx::setVertexBuffer(i, mesh.dvbufs[i]);
            }
            for (size_t i = 0; i < mesh.vbufs.size(); ++i) {
                bgfx::setVertexBuffer(i, mesh.vbufs[i]);
            }
            bgfx::setIndexBuffer(mesh.ibuf);
            bgfx::setState(
                BGFX_STATE_WRITE_MASK|
                BGFX_STATE_DEPTH_TEST_LESS
            );

            // print16f(glm::value_ptr(model));
            // r->model = rotate(r->model, 0.01f, {0.0f, 1.0f, 0.0f});
            bgfx::setTransform(glm::value_ptr(r->model));
            bgfx::submit(mm.mainView, r->program);
        }
    }


    // auto rview = mm.registry.view<NewRenderable>();
    // auto mview = mm.registry.view<NewMesh>();
    // rview.each([&mview](auto const entity, auto & renderable) {

    //     size_t i = 0;
    //     auto meshEntity = renderable.meshFirstChild;
    //     for (; i < renderable.meshCount; ++i) {
    //         NewMesh & mesh = mview.get<NewMesh>(meshEntity);

    //         for (size_t i = 0; i < mesh.dvbufs.size(); ++i) {
    //             bgfx::update(mesh.dvbufs[i], 0, mesh.dvbufRefs[i]);
    //             bgfx::setVertexBuffer(i, mesh.dvbufs[i]);
    //         }
    //         for (size_t i = 0; i < mesh.vbufs.size(); ++i) {
    //             bgfx::setVertexBuffer(i, mesh.vbufs[i]);
    //         }
    //         bgfx::setIndexBuffer(mesh.ibuf);
    //         bgfx::setState(
    //             BGFX_STATE_WRITE_MASK|
    //             BGFX_STATE_DEPTH_TEST_LESS
    //         );

    //         // print16f(glm::value_ptr(model));
    //         // r->model = rotate(r->model, 0.01f, {0.0f, 1.0f, 0.0f});
    //         bgfx::setTransform(glm::value_ptr(renderable.model));
    //         bgfx::submit(mm.mainView, renderable.program);

    //         meshEntity = mesh.nextSibling;
    //     }
    //     printc(ShowPrintOutput, "entity %d\n", (int)entity);
    // });
}

entt::entity RenderSystem::create2(bgfx::ProgramHandle program, char const * key) {
    auto e = mm.registry.create();
    mm.registry.emplace<Info>(e, key);
    mm.registry.emplace<NewRenderable>(e).program = program;
    return e;
}


size_t RenderSystem::create(bgfx::ProgramHandle program, char const * key) {
    if (poolNextFree >= poolSize) {
        printc(ShowPrintOutput, "WARNING: did not create renderable. Pool full.");
        return SIZE_MAX;
    }
    if (strcmp(key, "") != 0 && keyExists(key)) {
        printc(ShowPrintOutput, "WARNING: did not create renderable. Key already in use.");
        return SIZE_MAX;
    }

    size_t index = poolNextFree;
    printc(ShowPrintOutput, "CREATING AT %zu (%p)\n", index, &pool[index]);
    new (&pool[index]) Renderable();
    Renderable * renderable = at(index);
    renderable->program = program;
    renderable->active = true;
    renderables.push_back(index);
    if (strcmp(key, "") != 0) {
        renderable->key = key;
        keys.emplace(key, index);
    }

    setPoolNextFree(index+1);
    return index;
}

size_t RenderSystem::createFromGLTF(char const * filename) { 
    auto renderable = create(gltfProgram, filename);
    gltfLoader.load(filename, renderable);
    return renderable;
}

Renderable * RenderSystem::at(int index) {
    assert(index < poolSize && "Out of bounds.");
    return &pool[index];
}

Renderable * RenderSystem::at(char const * key) {
    return at(keys.at(key));
}

void RenderSystem::destroy(size_t index) {
    auto * r = at(index);
    r->active = false;
    for(auto i = renderables.begin(); i != renderables.end(); ++i) {
        if (*i == index) {
            renderables.erase(i);
            break;
        }
    }
    if (keyExists(r->key)) keys.erase(r->key);

    // we know this slot is free, but it shouldn't be the new free slot
    // if there is already a lower one
    if (poolNextFree > index)
        poolNextFree = index;
}

void RenderSystem::destroy(char const * key) {
    destroy(keys.at(key));
}


// 
// RENDERABLE UTILS
//

bool RenderSystem::keyExists(char const * key) {
    return (keys.find(key) != keys.end());
}

void RenderSystem::setPoolNextFree(size_t startFrom) {
    // if (poolNextFree < startFrom) return;
    printc(ShowPrintOutput, "LOOKING FOR NEXT FREE....\n");
    for (size_t i = startFrom; i < poolSize; ++i) {
        printc(ShowPrintOutput, "item in pool at %zu is %s\n", i, at(i)->active?"ACTIVE":"NOT ACTIVE");
        if (!at(i)->active) {
            poolNextFree = i;
            return;
        }
    }
    // no free slots found
    poolNextFree = SIZE_MAX;
}
