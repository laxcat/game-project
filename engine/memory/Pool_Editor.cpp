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

    Text("Pool<int> (%zu):", pool._size);

    // actions
    {
        // claim/release/reset
        static bool didClaim = false;
        static bool claimClicked = false;
        static size_t oppIndex = SIZE_MAX;
        static int * lastClaimed = nullptr;
        static char msg[128] = "";

        // claim
        if (Button("Claim")) {
            lastClaimed = pool.claim(&oppIndex);
            if (lastClaimed != nullptr) {
                *lastClaimed = 1;
                didClaim = true;
                snprintf(msg, 128, "Did claim %zu", oppIndex);
            }
            else {
                didClaim = false;
                snprintf(msg, 128, "Could not claim.");
            }

            claimClicked = true;
        }
        SameLine();

        // release last
        bool disableReleaseLast = (lastClaimed == nullptr);
        if (disableReleaseLast) BeginDisabled();
        if (Button("Release Last")) {
            bool success = pool.release(lastClaimed, &oppIndex);
            if (success) {
                lastClaimed = nullptr;
                snprintf(msg, 128, "Did release %zu", oppIndex);
            }
            else {
                snprintf(msg, 128, "Did not release.");
            }
        }
        if (disableReleaseLast) EndDisabled();
        SameLine();

        // reset
        if (Button("Reset")) {
            pool.reset();
            msg[0] = '\0';
        }
        SameLine();

        // release index
        static int releaseIndex = 0;
        PushItemWidth(90);
        InputInt("##ReleaseInput", &releaseIndex);
        PopItemWidth();
        SameLine();
        if (Button("Release")) {
            bool success = pool.release((size_t)releaseIndex);
            if (success) {
                snprintf(msg, 128, "Did release %d", releaseIndex);
            }
            else {
                snprintf(msg, 128, "Did not release.");
            }
        }

        // show action message
        TextUnformatted(msg);
    }

    // array contents
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
                Text("%d", pool.dataItems()[index]);
            }
            else {
                TableSetBgColor(ImGuiTableBgTarget_CellBg, GetColorU32(ImGuiCol_WindowBg));
            }
            ++index;
        }
    }
    EndTable();

}
