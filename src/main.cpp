#include "../engine/engine.h"
#include "../engine/dev/print.h"


void preInit()      { printl("preInit"); }
void postInit()     { printl("postInit"); }
void preEditor()    { printl("preEditor"); }
void postEditor()   { printl("postEditor"); }
void preDraw()      { printl("preDraw"); }
void postDraw()     { printl("postDraw"); }
void preResize()    { printl("preResize"); }
void postResize()   { printl("postResize"); }
void preShutdown()  { printl("preShutdown"); }
void postShutdown() { printl("postShutdown"); }


int main(int argc, char ** argv) {
    return main_desktop({
        .args = {argc, argv},
        // .preInit = preInit,
        // .postInit = postInit,
        // .preEditor = preEditor,
        // .postEditor = postEditor,
        // .preDraw = preDraw,
        // .postDraw = postDraw,
        // .preResize = preResize,
        // .postResize = postResize,
        // .preShutdown = preShutdown,
        // .postShutdown = postShutdown,
        // .transparentFramebuffer = true,
    });
}
