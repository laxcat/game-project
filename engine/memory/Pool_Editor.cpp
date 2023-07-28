#include "Pool.h"
#include "../common/imgui_bgfx_glfw/imgui_bgfx_glfw.h"
#include "../MrManager.h"

using namespace ImGui;

void Pool_editorCreate() {
    static int size = 10;
    SameLine();
    PushItemWidth(120);
    InputInt("Max##PoolBlock", &size);
    SameLine();
    if (Button("Create")) {
        mm.editor.clearMemEditWindow();
        Pool<int> * pool = mm.memMan.createPool<int>((size_t)size);
        if (pool) {
            mm.memMan.addTestAlloc(pool, "Pool<int> block (%d slots)", size);
        }
    }
    PopItemWidth();

}

void Pool_editorEditBlock(Pool<int> & pool) {

    // display array contents
    Text("Pool, (%zu):", pool._size);
    float colSize = 20.f;
    int nCols = 16;
    int nRows = pool._size / nCols + (pool._size % nCols != 0);
    int index = 0;
    BeginTable("SubBlocks", nCols);
    for (int row = 0; row < nRows; ++row) {
        TableNextRow();
        for (int col = 0; col < nCols; ++col) {
            TableSetColumnIndex(col);
            if (index < pool._size) {
                bool isFree = pool.isFree(index);
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

}
