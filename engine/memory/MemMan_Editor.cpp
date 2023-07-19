#include "MemMan.h"
#include <stdio.h>
#include "../common/imgui_bgfx_glfw/imgui_bgfx_glfw.h"
#include "../MrManager.h"

#if DEV_INTERFACE

using namespace ImGui;

// test allocs
struct Alloc {
    constexpr static size_t DescSize = 64;
    char desc[DescSize];
    void * ptr = nullptr;
};
static Alloc allocs[128] = {};
static uint16_t nAllocs = 0;

void addAlloc(void * ptr, char const * formatString = NULL, ...) {
    allocs[nAllocs++] = {.ptr=ptr};

    if (formatString) {
        va_list args;
        va_start(args, formatString);
        vsnprintf(allocs[nAllocs-1].desc, Alloc::DescSize, formatString, args);
        va_end(args);
    }
    else {
        allocs[nAllocs-1].desc[0] = '\0';
    }

}

void removeAlloc(uint16_t i) {
    if (i >= nAllocs) {
        return;
    }

    mm.memMan.request({.size=0, .ptr=allocs[i].ptr});

    for (int j = i + 1; j < nAllocs; ++j) {
        allocs[j-1] = allocs[j];
    }
    allocs[--nAllocs] = {};
}

void removeAllAllocs() {
    while(nAllocs) {
        removeAlloc(nAllocs-1);
    }
}

void MemMan::editor() {
    if (!CollapsingHeader("Mem 2", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    // MEMMAN NOT INITIALIZED ----------------------------------------------- //
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
        return;
    }

    // MEMMAN IS INITIALIZED ------------------------------------------------ //

    // MEMMAN GENERAL INFO
    Text("MemMan Initialized [%s]", mm.byteSizeStr(size()));
    Text("%p - %p", data(), data() + size());
    if (Button("Shutdown")) {
        removeAllAllocs();
        shutdown();
    }
    Dummy(ImVec2(0.0f, 10.0f));
    Separator();

    // TEST ALLOCATIONS
    if (nAllocs < 128) {
        // ALLOCATE GENERIC
        PushItemWidth(90);
        TextUnformatted("Allocate Generic:");
        {
            static int sizeAlign[] = {4, 0};

            // create a test allocation
            if (nAllocs < 128) {
                InputInt2("Size/align##alloc", sizeAlign);
                SameLine();
                if (Button("Allocate")) {
                    void * ptr = request({.size=(size_t)sizeAlign[0], .align=(size_t)sizeAlign[1]});
                    if (ptr) {
                        addAlloc(ptr, "%d-byte generic", sizeAlign[0]);
                    }
                }
            }
        }
        PopItemWidth();
        Dummy(ImVec2(0.0f, 10.0f));

        // ALLOCATE BLOCK
        PushItemWidth(90);
        TextUnformatted("Manually Create Block Object:");
        static MemBlockType selectedType = MEM_BLOCK_ARRAY;
        if (BeginCombo("###MemBlockType", memBlockTypeStr(selectedType))) {
            for (int i = MEM_BLOCK_CLAIMED; i <= MEM_BLOCK_GOBJ; ++i) {
                if (Selectable(memBlockTypeStr((MemBlockType)i))) {
                    selectedType = (MemBlockType)i;
                }
            }
            EndCombo();
        }
        PopItemWidth();

        // INPUT PARAMETERS FOR EACH BLOCK TYPE
        switch (selectedType) {
        case MEM_BLOCK_CLAIMED: {
            static int sizeAlign[] = {1024, 0};
            SameLine();
            PushItemWidth(120);
            InputInt2("Size/align##ClaimedBlock", sizeAlign);
            PopItemWidth();
            SameLine();
            if (Button("Create")) {
                mm.editor.clearMemEditWindow();
                BlockInfo * block = create(sizeAlign[0], sizeAlign[1]);
                if (block) {
                    addAlloc(block->data(), "Generic block (%zu bytes)", block->dataSize());
                }
            }
            break;
        }
        case MEM_BLOCK_ARRAY: {
            static int max = 10;
            SameLine();
            PushItemWidth(120);
            InputInt("Max##ArrayBlock", &max);
            SameLine();
            if (Button("Create")) {
                mm.editor.clearMemEditWindow();
                Array<int> * arr = createArray<int>((size_t)max);
                if (arr) {
                    addAlloc(arr, "Array<int> block (%d-item max)", max);
                }
            }
            break;
        }
        case MEM_BLOCK_POOL:
        case MEM_BLOCK_STACK:
        case MEM_BLOCK_FILE:
        case MEM_BLOCK_GOBJ:
        default: {
            TextUnformatted("Not implemented yet.");
        }}
    }

    // LIST TEST ALLOCS
    if (nAllocs) {
        Dummy(ImVec2(0.0f, 10.0f));
        TextUnformatted("Test Allocs:");

        for (int i = 0; i < nAllocs; ++i) {
            PushID(i);
            Text("%p (%s)", allocs[i].ptr, allocs[i].desc);
            SameLine();
            if (Button("Release")) {
                removeAlloc(i);
            }
            PopID();
        }

        if (Button("Release All")) {
            removeAllAllocs();
        }
    }

    Dummy(ImVec2(0.0f, 10.0f));

    Separator();
    Text("%zu Blocks", blockCountForDisplayOnly());
    Separator();

    // LIST BLOCKS ---------------------------------------------------------- //
    ImGuiStyle * style = &ImGui::GetStyle();
    ImVec4 defaultTextColor = style->Colors[ImGuiCol_Text];
    int blockIndex = 0;
    for (MemMan::BlockInfo * b = firstBlock(); b; b = nextBlock(b)) {
        PushID(blockIndex);

        void * basePtr = (void *)((MemMan::BlockInfo const *)b)->basePtr();

        bool isTestAlloc = false;
        int testAllocIndex = 0;
        for (int i = 0; i < nAllocs; ++i) {
            if (b->data() == allocs[i].ptr) {
                isTestAlloc = true;
                testAllocIndex = i;
                break;
            }
        }

        // calc block name
        char * str = mm.tempStr(128);
        snprintf(str, 128, "%03d (%p): %s Block [%s][%s]",
            blockIndex,
            basePtr,
            memBlockTypeStr(b->type()),
            mm.byteSizeStr(b->paddingSize()),
            mm.byteSizeStr(b->dataSize())
        );

        bool isSelected = (b == mm.editor.memWin.ptr);
        ImVec2 selSize = ImVec2{GetWindowContentRegionMax().x - 50.f, 0.f};

        if (!isTestAlloc) {
            selSize = {};
        }

        if (b->type() == MEM_BLOCK_FREE) {
            style->Colors[ImGuiCol_Text] = ImVec4(0.6f, 0.7f, 1.0f, 1.00f);
        }
        else if (b->type() == MEM_BLOCK_CLAIMED) {
            style->Colors[ImGuiCol_Text] = ImVec4(0.9f, 0.5f, 0.5f, 1.00f);
        }

        if (Selectable(str, isSelected, 0, selSize)) {
            if (isSelected) {
                mm.editor.clearMemEditWindow();
            }
            else {
                mm.editor.showMemEditWindow(str, b, b->blockSize());
            }
        }

        // show free button
        if (isTestAlloc) {
            SameLine();
            if (Button("Free")) {
                mm.editor.clearMemEditWindow();
                // release(b);
                removeAlloc(testAllocIndex);
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
        }

        // ARRAY
        if (b->type() == MEM_BLOCK_ARRAY) {
            auto & arr = *(Array<int> *)b->data();

            // append
            {
                bool disabled = arr.bufferFull();
                if (disabled) BeginDisabled();

                TextUnformatted("Append Item");
                static int appendValue = 0;
                InputInt("value##append", &appendValue);
                SameLine();
                if (Button("Append###ArrayAppend")) {
                    arr.append(appendValue);
                }

                // insert
                TextUnformatted("Insert Item");
                static int insertValue = 0;
                static int insertIndex = -1;
                if (insertIndex == -1) {
                    insertIndex = (int)arr.size();
                }
                InputInt("value##insert", &insertValue);
                SameLine();
                InputInt("index##insert", &insertIndex);
                SameLine();
                if (Button("Insert###ArrayInsert")) {
                    arr.insert((size_t)insertIndex, insertValue);
                }

                if (disabled) EndDisabled();
            }

            // remove
            {
                bool disabled = (arr.size() == 0);
                if (disabled) BeginDisabled();

                TextUnformatted("Remove Items");
                static int removeParams[2] = {0, 1};
                InputInt2("index/count", removeParams);
                size_t index = (size_t)removeParams[0];
                size_t count = (size_t)removeParams[1];
                bool badInput = index + count > arr.size();

                if (badInput) BeginDisabled();

                SameLine();
                if (Button("Remove###ArrayRemove")) {
                    arr.remove(index, count);
                }

                if (badInput) EndDisabled();
                if (disabled) EndDisabled();
            }


            // copy
            TextUnformatted("Copy Items");
            static int copyParams[3];
            InputInt3("dst/src/count", copyParams);
            SameLine();
            if (Button("Copy###ArrayCopy")) {
                arr.copy((size_t)copyParams[0], (size_t)copyParams[1], (size_t)copyParams[2]);
            }

            // display array contents
            Text("Array, (%zu/%zu):", arr.size(), arr.maxSize());
            int cols = 16;
            int rows = arr.maxSize() / cols + (arr.maxSize() % cols != 0);
            int i = 0;
            BeginTable("SubBlocks", cols, ImGuiTableFlags_Borders);
            for (int i = 0; i < arr.maxSize(); ++i) {
                TableNextColumn();
                TableSetBgColor(ImGuiTableBgTarget_CellBg, 0xff333333);
                if (i == arr.size()) {
                    style->Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
                }
                Text("%d", arr.data()[i]);
            }
            style->Colors[ImGuiCol_Text] = defaultTextColor;
            EndTable();
        }

        style->Colors[ImGuiCol_Text] = defaultTextColor;
        Separator();
        ++blockIndex;

        PopID();
    }
}
#endif // DEV_INTERFACE
