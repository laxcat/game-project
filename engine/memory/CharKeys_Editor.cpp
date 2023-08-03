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

static void drawTree(CharKeys::Node * n) {
    if (n == nullptr) {
        TextUnformatted("(null)");
        return;
    }

    SetNextItemOpen(true, ImGuiCond_Once);

    char const * title = mm.frameFormatStr("Node(%s, %zu)", n->key, n->ptr);
    if (TreeNode(title)) {
        TextUnformatted("L:");
        SameLine();
        drawTree(n->left);
        TextUnformatted("R:");
        SameLine();
        drawTree(n->right);
        TreePop();
    }
}

static void listKeys(CharKeys::Node * n) {
    if (n == nullptr) return;
    // inorder traversal
    listKeys(n->left);
    TextUnformatted(n->key);
    listKeys(n->right);
}

void CharKeys::editorEditBlock() {

    Text("CharKeys (%zu/%zu):", nNodes(), _size);

    Dummy(ImVec2(0.0f, 10.0f));

    bool disableInput = isFull();
    static constexpr size_t msgMax = 64;
    static char msg[msgMax];

    if (disableInput) BeginDisabled();
    PushItemWidth(90);
    static char key[16];
    static int ptr = 1;
    TextUnformatted("Create New Node");
    InputText("##key", key, 16, ImGuiInputTextFlags_CharsNoBlank);
    SameLine();
    InputInt("##ptr", &ptr);
    SameLine();
    TextUnformatted("key/ptr");
    PopItemWidth();
    if (Button("Create")) {
        CharKeys::Status status = add(key, (void *)(size_t)ptr);
        switch (status) {
        case CharKeys::SUCCESS:         snprintf(msg, msgMax, "Added node");                    break;
        case CharKeys::BUFFER_FULL:     snprintf(msg, msgMax, "Buffer full; did not add.");     break;
        case CharKeys::DUPLICATE_KEY:   snprintf(msg, msgMax, "Duplicate key; did not add.");   break;
        }
    }
    if (disableInput) EndDisabled();
    TextUnformatted(msg);

    Dummy(ImVec2(0.0f, 10.0f));

    TextUnformatted("Nodes");
    drawTree(_root);

    Dummy(ImVec2(0.0f, 10.0f));

    TextUnformatted("Key list");
    listKeys(_root);
}
