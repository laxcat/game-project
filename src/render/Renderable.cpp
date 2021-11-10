#include "Renderable.h"
#include "../MrManager.h"
#include <glm/gtc/type_ptr.hpp>


void Renderable::draw() {
    for (auto const & mesh : meshes) {
        for (auto const & dvbuf : mesh.dvbufs) {
            bgfx::update(dvbuf, 0, mesh.vertsRef());
            bgfx::setVertexBuffer(mm.mainView, dvbuf);
        }
        for (auto const & vbuf : mesh.vbufs) {
            bgfx::setVertexBuffer(mm.mainView, vbuf);
        }
        bgfx::setIndexBuffer(mesh.ibuf);
        bgfx::setState(
            BGFX_STATE_WRITE_MASK|
            BGFX_STATE_DEPTH_TEST_LESS
        );

        // print16f(glm::value_ptr(model));
        bgfx::setTransform(glm::value_ptr(model));
        bgfx::submit(mm.mainView, program);
    }

}