#include "Renderable.h"
#include "../MrManager.h"


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
        bgfx::setState(BGFX_STATE_WRITE_RGB);
        bgfx::submit(mm.mainView, program);
    }

}