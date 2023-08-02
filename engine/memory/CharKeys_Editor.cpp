#include "CharKeys.h"
#include "../common/imgui_bgfx_glfw/imgui_bgfx_glfw.h"
#include "../MrManager.h"

using namespace ImGui;

void CharKeys::editorCreate() {
    static int max = 10;
    SameLine();
    PushItemWidth(120);
    InputInt("Max##CharKeysBlock", &max);
    SameLine();
    if (Button("Create")) {
        mm.editor.clearMemEditWindow();
        CharKeys * keys = mm.memMan.createCharKeys((size_t)max);
        if (keys) {
            mm.memMan.addTestAlloc(keys, "CharKeys<int> block (%d max)", max);
        }
    }
    PopItemWidth();
}

static void drawTree(CharKeys::Node * root) {

}

void CharKeys::editorEditBlock() {

}
