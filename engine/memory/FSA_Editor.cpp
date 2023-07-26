#include "FSA.h"
#include "../common/imgui_bgfx_glfw/imgui_bgfx_glfw.h"
#include "../MrManager.h"

using namespace ImGui;

void FSA::editorEditBlock() {

    Indent();
    for (uint16_t fsaGroup = 0; fsaGroup < FSA::Max; ++fsaGroup) {
        uint16_t nSubBlocks = subBlockCountForGroup(fsaGroup);
        if (nSubBlocks == 0) continue;
        PushID(fsaGroup);
        char const * titleStr = mm.frameFormatStr(64, "%5d %d-byte sub-blocks (0x%06X)",
            nSubBlocks,
            1 << (fsaGroup + 1),
            (uint32_t)((size_t)freeListPtrForGroup(fsaGroup) & 0xffffff));
        if (CollapsingHeader(titleStr)) {
            int nCols = 16;
            float colSize = 20.f;
            int rowCount = nSubBlocks / nCols + (nSubBlocks % nCols != 0);
            int subBlockIndex = 0;
            BeginTable("SubBlocks", nCols);
            for (int row = 0; row < rowCount; ++row) {
                TableNextRow();
                for (int col = 0; col < nCols; ++col) {
                    TableSetColumnIndex(col);
                    if (subBlockIndex < nSubBlocks) {
                        bool isFree = this->isFree(fsaGroup, subBlockIndex);
                        uint32_t color = isFree ? 0xff888888 : 0xff993333;
                        TableSetBgColor(ImGuiTableBgTarget_CellBg, color);
                        Text("%d", subBlockIndex);
                    }
                    else {
                        TableSetBgColor(ImGuiTableBgTarget_CellBg, GetColorU32(ImGuiCol_WindowBg));
                    }
                    ++subBlockIndex;
                }
            }
            EndTable();
        }
        PopID();
    }
    Unindent();
}
