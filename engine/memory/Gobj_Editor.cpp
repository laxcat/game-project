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

void Gobj::drawNode(Node * n) {
        // if (TreeNode("Node", "Node %zu (%s)", n - nodes, n->name)) {
        //     for (uint16_t i = 0; i < n->nChildren; ++i) {
        //         drawNode(n->children + i);
        //     }
        //     TreePop();
        // }
}

void Gobj::editorEditBlock() {
    if (CollapsingHeader("Info")) {
        TextUnformatted(printToFrameStack());
    }
    #if DEBUG
    if (CollapsingHeader("JSON")) {
        TextUnformatted(jsonStr);
    }
    #endif // DEBUG
    if (CollapsingHeader("Hierarchy")) {
        for (uint16_t i = 0; i < counts.scenes; ++i) {
            Scene const & s = scenes[i];
            if (TreeNode("Scene", "Scene %d (%s)", i, s.name)) {
                printl("scene nNode %d", s.nNodes);
                for (uint16_t j = 0; j < s.nNodes; ++j) {
                    printl("scene node %d: %d", i, s.nodes[i]);
                    // drawNode(nodes + s.nodes[i]);
                }
                TreePop();
            }
        }
    }
}
