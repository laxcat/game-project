#include "Renderable.h"
#include "../MrManager.h"


void Renderable::draw() {
    fprintf(stderr, "do it!\n");
    for (auto const & mesh : meshes) {
        for (auto const & vbuf : mesh.dvbufs) {
            bgfx::setVertexBuffer(mm.mainView, mesh.dvbufs[0]);
        }
        bgfx::setIndexBuffer(mesh.ibuf);
        bgfx::setState(BGFX_STATE_WRITE_RGB);
        bgfx::submit(mm.mainView, program);
    }

}