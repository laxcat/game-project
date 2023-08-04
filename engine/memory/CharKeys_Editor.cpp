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
        TextUnformatted("-");
        return;
    }

    SetNextItemOpen(true, ImGuiCond_Once);

    char const * title = mm.frameFormatStr("%s (%zu)", n->key, n->ptr);
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
    Text("%s,", n->key);
    SameLine();
    listKeys(n->right);
}

void CharKeys::editorEditBlock() {
    // info
    Text("CharKeys (%zu/%zu):", nNodes(), _size);

    Dummy(ImVec2(0.0f, 10.0f));

    bool disableInsert = isFull();
    bool disableRemove = (pool()->_nClaimed == 0);
    static constexpr size_t msgMax = 64;
    static char msg[msgMax];
    static int inputFlags = ImGuiInputTextFlags_CharsNoBlank |
                            ImGuiInputTextFlags_AutoSelectAll |
                            ImGuiInputTextFlags_EnterReturnsTrue;

    // insert
    {
        if (disableInsert) BeginDisabled();
        PushItemWidth(120);
        static char key[KEY_MAX];
        static int ptr = 1;
        TextUnformatted("Insert New Node");
        bool didPressEnter = InputText("##insertKey", key, KEY_MAX, inputFlags);
        if (didPressEnter && !disableInsert) {
            SetKeyboardFocusHere(-1);
        }
        SameLine();
        InputInt("##ptr", &ptr);
        SameLine();
        TextUnformatted("key/ptr");
        PopItemWidth();
        SameLine();
        // prevent keyboard repeat
        if (didPressEnter) {
            bool retFirst = IsKeyPressed(ImGuiKey_Enter, false) || IsKeyPressed(ImGuiKey_KeypadEnter, false);
            bool retRepeat = IsKeyPressed(ImGuiKey_Enter, true) || IsKeyPressed(ImGuiKey_KeypadEnter, true);
            if (!retFirst && retRepeat) {
                didPressEnter = false;
            }
        }
        if (Button("Insert") || didPressEnter) {
            CharKeys::Status status = insert(key, (void *)(size_t)ptr);
            switch (status) {
            case CharKeys::SUCCESS:         snprintf(msg, msgMax, "Inserted node");                 break;
            case CharKeys::BUFFER_FULL:     snprintf(msg, msgMax, "Buffer full; did not insert");   break;
            case CharKeys::DUPLICATE_KEY:   snprintf(msg, msgMax, "Duplicate key; did not insert"); break;
            }
        }
        if (disableInsert) EndDisabled();
        Dummy(ImVec2(0.0f, 10.0f));
    }

    // remove
    {
        if (disableRemove) BeginDisabled();
        PushItemWidth(120);
        static char key[KEY_MAX];
        TextUnformatted("Remove Node");
        bool didPressEnter = InputText("key##removeKey", key, KEY_MAX, inputFlags);
        if (didPressEnter && !disableRemove) {
            SetKeyboardFocusHere(-1);
        }
        PopItemWidth();
        SameLine();
        // prevent keyboard repeat
        if (didPressEnter) {
            bool retFirst = IsKeyPressed(ImGuiKey_Enter, false) || IsKeyPressed(ImGuiKey_KeypadEnter, false);
            bool retRepeat = IsKeyPressed(ImGuiKey_Enter, true) || IsKeyPressed(ImGuiKey_KeypadEnter, true);
            if (!retFirst && retRepeat) {
                didPressEnter = false;
            }
        }
        if (Button("Remove") || didPressEnter) {
            bool didRemove = remove(key);
            if (didRemove) { snprintf(msg, msgMax, "Removed node"); }
            else           { snprintf(msg, msgMax, "Did not remove node"); }
        }
        if (disableRemove) EndDisabled();
        Dummy(ImVec2(0.0f, 10.0f));
    }

    // show message
    TextUnformatted(msg);
    if (msg[0]) {
        Dummy(ImVec2(0.0f, 10.0f));
    }

    // draw tree
    TextUnformatted("Nodes");
    drawTree(_root);
    Dummy(ImVec2(0.0f, 10.0f));

    // list keys
    TextUnformatted("Key list");
    listKeys(_root);
    TextUnformatted("");
    Dummy(ImVec2(0.0f, 10.0f));

    // show pool
    if (CollapsingHeader("Pool")) {
        PoolT & pool = *this->pool();

        float colSize = 20.f;
        int nCols = 16;
        int nRows = pool._size / nCols + (pool._size % nCols != 0);
        int index = 0;
        BeginTable("Pool", nCols);
        for (int row = 0; row < nRows; ++row) {
            TableNextRow();
            for (int col = 0; col < nCols; ++col) {
                TableSetColumnIndex(col);
                if (index < pool._size) {
                    bool isFree = pool.isFree(index);
                    uint32_t color = isFree ? 0xff888888 : 0xff993333;
                    TableSetBgColor(ImGuiTableBgTarget_CellBg, color);
                    TextUnformatted(pool.dataItems()[index].key);
                }
                else {
                    TableSetBgColor(ImGuiTableBgTarget_CellBg, GetColorU32(ImGuiCol_WindowBg));
                }
                ++index;
            }
        }
        EndTable();
    }
}
