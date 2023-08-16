#include "Gobj.h"
#include <nfd.h>
#include "../common/imgui_bgfx_glfw/imgui_bgfx_glfw.h"
#include "../MrManager.h"

using namespace ImGui;

void Gobj::editorCreate() {
    SameLine();

    if (Button("Create From GLTF")) {
        nfdchar_t * outPath = NULL;
        nfdresult_t result = NFD_OpenDialog(NULL, "/Users/Shared/Dev/gltf_assets", &outPath);

        if (result == NFD_OKAY) {
            Gobj * g = mm.memMan.createGobj(outPath);
            if (g) {
                mm.memMan.addTestAlloc(g, "Gobj: %s", outPath);
            }
            free(outPath);

        }
        else if (result == NFD_CANCEL) {}
        else {
            fprintf(stderr, "Error: %s\n", NFD_GetError());
        }
    }
}

void Gobj::editorEditBlock() {
    if (CollapsingHeader("Info")) {
        TextUnformatted(printToFrameStack());
    }
}
