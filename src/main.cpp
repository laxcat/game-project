#include "../engine/engine.h"
#include "../engine/dev/print.h"


void preInit()      { print("preInit\n"); }
void postInit()     { print("postInit\n"); }
void preEditor()    { print("preEditor\n"); }
void postEditor()   { print("postEditor\n"); }
void preDraw()      { print("preDraw\n"); }
void postDraw()     { print("postDraw\n"); }
void preResize()    { print("preResize\n"); }
void postResize()   { print("postResize\n"); }
void preShutdown()  { print("preShutdown\n"); }
void postShutdown() { print("postShutdown\n"); }


int main() {
    return main_desktop({
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
