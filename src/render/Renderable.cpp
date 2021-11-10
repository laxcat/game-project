#include "Renderable.h"
#include "../MrManager.h"
#include <glm/gtc/type_ptr.hpp>


void Renderable::draw() {
    for (auto const & mesh : meshes) {
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
        model = rotate(model, 0.03f, {0.0f, 1.0f, 0.0f});
        bgfx::setTransform(glm::value_ptr(model));
        bgfx::submit(mm.mainView, program);
    }

}