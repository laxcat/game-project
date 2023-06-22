#include "MemMan2.h"
#include <stdio.h>
#include "../common/imgui_bgfx_glfw/imgui_bgfx_glfw.h"
#include "../MrManager.h"

#if DEV_INTERFACE

using namespace ImGui;

void MemMan2::editor() {
    if (!CollapsingHeader("Mem 2", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    // memman not initialized ----------------------------------------------- //
    if (!firstBlock()) {
        static int size = mm.setup.memManSize;
        static MemManFSASetup fsaSetup = mm.setup.memManFSA;
        InputInt("Init size", &size, 1024, 1024*1024);
        TextUnformatted("FSA sub-block counts");
        PushItemWidth(100);
        uint16_t fsaCountStep = 8;
        for (int fsaI = 0; fsaI < MemManFSASetup::Max; ++fsaI) {
            char * title = mm.tempStr(64);
            snprintf(title, 64, "%d-byte sub-blocks", 1 << (fsaI+1));
            InputScalar(title, ImGuiDataType_U16, &fsaSetup.nSubBlocks[fsaI], &fsaCountStep);
        }
        PopItemWidth();
        if (Button("Init")) {
            // sanitize fsa sub-block counts
            for (int fsaI = 0; fsaI < MemManFSASetup::Max; ++fsaI) {
                fsaSetup.nSubBlocks[fsaI] &= 0xfff8;
            }
            EngineSetup setup;
            setup.memManSize = size;
            setup.memManFSA = fsaSetup;
            init(setup);
        }
    }

    // memman is initialized ------------------------------------------------ //
    else {
        // test allocs
        struct Allocs {
            size_t size = 0;
            void * ptr = nullptr;
        };
        static Allocs allocs[128] = {};
        static int nAllocs = 0;

        Text("%p - %p", data(), data() + size());
        SameLine();
        if (Button("Shutdown")) {
            shutdown();
            for (int i = 0; i < nAllocs; ++i) {
                allocs[i] = {};
            }
            nAllocs = 0;
        }

        PushItemWidth(90);

        TextUnformatted("Allocate Generic:");
        {
            static int sizeAlign[] = {4, 0};

            if (nAllocs < 128) {
                InputInt2("Size/align##alloc", sizeAlign);
                // TODO: redo using reqeust interface
                // SameLine();
                // if (Button("Allocate")) {
                //     MemMan2::BlockInfo * block;
                //     void * ptr = alloc(sizeAlign[0], sizeAlign[1], &block);
                //     if (block != firstBlock()) {
                //         mm.editor.clearMemEditWindow();
                //     }
                //     if (ptr) {
                //         allocs[nAllocs++] = {(size_t)sizeAlign[0], (void *)ptr};
                //     }
                // }
            }

            // quick list of test allocs
            for (int i = 0; i < nAllocs; ++i) {
                PushID(i);
                Text(
                    "%zu-byte alloc (0x%06X)",
                    allocs[i].size, (uint32_t)((size_t)allocs[i].ptr & 0xffffff)
                );
                // TODO: redo using request interface
                // SameLine();
                // if (Button("Release")) {
                //     destroy(allocs[i].ptr);
                //     for (int j = i + 1; j < nAllocs; ++j) {
                //         allocs[j-1] = allocs[j];
                //     }
                //     allocs[nAllocs-1] = {};
                //     --nAllocs;
                // }
                PopID();
            }
        }
        Dummy(ImVec2(0.0f, 10.0f));

        TextUnformatted("Manually Create Block:");

        static MemBlockType selectedType = MEM_BLOCK_CLAIMED;

        if (BeginCombo("###MemBlockType", memBlockTypeStr(selectedType))) {
            for (int i = MEM_BLOCK_CLAIMED; i <= MEM_BLOCK_GOBJ; ++i) {
                if (i == MEM_BLOCK_FSA) continue;

                if (Selectable(memBlockTypeStr((MemBlockType)i))) {
                    selectedType = (MemBlockType)i;
                }
            }
            EndCombo();
        }

        SameLine();

        PopItemWidth();
        PushItemWidth(120);

        switch (selectedType) {
        case MEM_BLOCK_CLAIMED: {
            static int sizeAlign[] = {1024, 0};
            InputInt2("Size/align##block", sizeAlign);
            SameLine();
            if (Button("Create")) {
                mm.editor.clearMemEditWindow();
                create(sizeAlign[0], sizeAlign[1]);
            }
            break;
        }
        case MEM_BLOCK_POOL:
        case MEM_BLOCK_STACK:
        case MEM_BLOCK_FILE:
        case MEM_BLOCK_GOBJ:
        case MEM_BLOCK_FSA:
        default: {
            TextUnformatted("Not implemented yet.");
        }}

        PopItemWidth();

        Dummy(ImVec2(0.0f, 10.0f));

        Text("%zu Blocks", blockCountForDisplayOnly());
        Separator();
    }

    // list blocks ---------------------------------------------------------- //
    ImGuiStyle * style = &ImGui::GetStyle();
    ImVec4 defaultTextColor = style->Colors[ImGuiCol_Text];
    int blockIndex = 0;
    for (MemMan2::BlockInfo * b = firstBlock(); b; b = nextBlock(b)) {
        PushID(blockIndex);

        void * basePtr = (void *)((MemMan2::BlockInfo const *)b)->basePtr();

        // calc block name
        char * str = mm.tempStr(128);
        snprintf(str, 128, "%03d (0x%06X): %s Block [%s][%s]",
            blockIndex,
            (uint32_t)((size_t)basePtr & 0xffffff),
            memBlockTypeStr(b->type()),
            mm.byteSizeStr(b->paddingSize()),
            mm.byteSizeStr(b->dataSize())
        );

        bool isSelected = (b == mm.editor.memWin.ptr);
        ImVec2 selSize = ImVec2{GetWindowContentRegionMax().x - 50.f, 0.f};

        if (b->type() == MEM_BLOCK_FREE) {
            style->Colors[ImGuiCol_Text] = ImVec4(0.6f, 0.7f, 1.0f, 1.00f);
            selSize = {};
        }
        else if (b->type() == MEM_BLOCK_CLAIMED) {
            style->Colors[ImGuiCol_Text] = ImVec4(0.9f, 0.5f, 0.5f, 1.00f);
        }

        if (Selectable(str, isSelected, 0, selSize)) {
            if (isSelected) {
                mm.editor.clearMemEditWindow();
            }
            else {
                snprintf(mm.editor.memWin.title, MemEditWindow::TitleSize, "%s###memEditWindow", str);
                mm.editor.showMemEditWindow(b, b->blockSize());
            }
        }

        // show free button
        if (b->type() != MEM_BLOCK_FREE && b->type() != MEM_BLOCK_FSA) {
            SameLine();
            if (Button("Free")) {
                mm.editor.clearMemEditWindow();
                b = release(b);
            }
        }

        // sub type specifics
        // FSA
        if (b->type() == MEM_BLOCK_FSA) {
            FSA * fsa = (FSA *)b->data();
            Indent();
            for (uint16_t fsaGroup = 0; fsaGroup < FSA::Max; ++fsaGroup) {
                uint16_t nSubBlocks = fsa->subBlockCountForGroup(fsaGroup);
                if (nSubBlocks == 0) continue;
                PushID(fsaGroup);
                char * titleStr = mm.tempStr(64);
                snprintf(titleStr, 64, "%5d %d-byte sub-blocks (0x%06X)",
                    nSubBlocks,
                    1 << (fsaGroup + 1),
                    (uint32_t)((size_t)fsa->freeListPtrForGroup(fsaGroup) & 0xffffff));
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
                                bool isFree = fsa->isFree(fsaGroup, subBlockIndex);
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

            // uint16_t nSubBlocks = fsa->nSubBlocks();
            // Text("%d %d-byte sublocks", nSubBlocks, fsa->subBlockSize());
            // int nCols = 16;
            // float colSize = 20.f;
            // int rowCount = nSubBlocks / nCols + (nSubBlocks % nCols != 0);
            // int cellIndex = 0;
            // BeginTable("SubBlocks", nCols);
            // for (int row = 0; row < rowCount; ++row) {
            //     TableNextRow();
            //     for (int col = 0; col < nCols; ++col) {
            //         TableSetColumnIndex(col);
            //         if (cellIndex < nSubBlocks) {
            //             uint32_t color = fsa->isFree(cellIndex) ? 0xff888888 : 0xff333333;
            //             TableSetBgColor(ImGuiTableBgTarget_CellBg, color);
            //             Text("%d", cellIndex);
            //         }
            //         else {
            //             TableSetBgColor(ImGuiTableBgTarget_CellBg, GetColorU32(ImGuiCol_WindowBg));
            //         }
            //         ++cellIndex;
            //     }
            // }
            // EndTable();
        }

        style->Colors[ImGuiCol_Text] = defaultTextColor;
        Separator();
        ++blockIndex;

        PopID();
    }
}
#endif // DEV_INTERFACE
