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

void Gobj::drawNode(Node * node) {
    ImGuiTreeNodeFlags flags = 0;
    if (node->nChildren == 0) flags |= ImGuiTreeNodeFlags_Leaf;
    if (!TreeNodeEx(node, flags, "Node %zu (%s)", node - nodes, node->name)) {
        return;
    }
    for (uint16_t i = 0; i < node->nChildren; ++i) {
        drawNode(node->children[i]);
    }
    TreePop();
}

void Gobj::editorEditBlock() {
    if (scenes) {
        if (BeginCombo("Scene", scene->name)) {
            for (uint16_t i = 0; i < counts.scenes; ++i) {
                if (Selectable(scenes[i].name)) {
                    scene = scenes + i;
                }
            }
            EndCombo();
        }
    }

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
            Scene const * scene = scenes + i;
            ImGuiTreeNodeFlags flags = 0;
            if (scene->nNodes == 0) flags |= ImGuiTreeNodeFlags_Leaf;
            if (!TreeNodeEx(scene, flags, "Scene %d (%s)", i, scene->name)) {
                continue;
            }
            for (uint16_t j = 0; j < scene->nNodes; ++j) {
                drawNode(scene->nodes[j]);
            }
            TreePop();
        }
    }
}
