#include "RenderSystem.h"
#include <glm/gtc/type_ptr.hpp>
#include "../MrManager.h"


void RenderSystem::init() {
    gltfProgram = mm.memSys.loadProgram("vs_tester", "fs_tester");
}

void RenderSystem::draw() {
    // fprintf(stderr, "renderable count: %zu\n", renderables.size());
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
    //     printf("entity %d\n", (int)entity);
    // });
}

entt::entity RenderSystem::create2(bgfx::ProgramHandle program, char const * key) {
    auto e = mm.registry.create();
    mm.registry.emplace<Info>(e, key);
    mm.registry.emplace<NewRenderable>(e).program = program;
    return e;
}
