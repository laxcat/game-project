#include "FreeList.h"
#include <assert.h>
#include "../common/imgui_bgfx_glfw/imgui_bgfx_glfw.h"
#include "mem_utils.h"
#include "../MrManager.h"


using namespace ImGui;

void FreeList::editorCreate() {
    static int max = 64;
    SameLine();
    PushItemWidth(120);
    InputInt("Max##FreeListBlock", &max);
    SameLine();
    if (Button("Create##freeList")) {
        mm.editor.clearMemEditWindow();
        FreeList * list = mm.memMan.createFreeList((size_t)max);
        if (list) {
            mm.memMan.addTestAlloc(list, "FreeList block (%zu items)", list->_nItems);
        }
    }
    PopItemWidth();
}

void FreeList::editorEditBlock() {
    Indent();
    Text("FreeList, %zu max", _nItems);

    static bool didClaim = false;
    static bool everClicked = false;
    static size_t claimed = _nItems;
    if (Button("Claim")) {
        didClaim = claim(&claimed);
        everClicked = true;
    }
    SameLine();
    if (Button("Claim All")) {
        claimAll();
    }
    SameLine();
    if (Button("Reset")) {
        reset();
    }
    SameLine();
    static int releaseIndex = 0;
    PushItemWidth(90);
    InputInt("##ReleaseInput", &releaseIndex);
    PopItemWidth();
    SameLine();
    if (Button("Release")) {
        release((size_t)releaseIndex);
    }

    if (everClicked) {
        if (didClaim) {
            Text("Did claim %zu", claimed);
        }
        else {
            TextUnformatted("Cound not claim.");
        }
    }

    int nCols = 16;
    float colSize = 20.f;
    int rowCount = _nItems / nCols + (_nItems % nCols != 0);
    int index = 0;
    BeginTable("FreeList", nCols);
    for (int row = 0; row < rowCount; ++row) {
        TableNextRow();
        for (int col = 0; col < nCols; ++col) {
            TableSetColumnIndex(col);
            if (index < _nItems) {
                bool isFree = this->isFree(index);
                uint32_t color = isFree ? 0xff888888 : 0xff993333;
                TableSetBgColor(ImGuiTableBgTarget_CellBg, color);
                Text("%d", index);
            }
            else {
                TableSetBgColor(ImGuiTableBgTarget_CellBg, GetColorU32(ImGuiCol_WindowBg));
            }
            ++index;
        }
    }
    EndTable();
    Unindent();
}
