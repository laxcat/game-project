#include "MemMan.h"
#include <stdio.h>
#include "../common/imgui_bgfx_glfw/imgui_bgfx_glfw.h"
#include "../MrManager.h"
#include "mem_utils.h"
#include "FSA.h"
#include "File.h"
#include "FreeList.h"
#include "CharKeys.h"

#if DEV_INTERFACE

using namespace ImGui;

void MemMan::editor() {
    if (!CollapsingHeader("Memory"/*, ImGuiTreeNodeFlags_DefaultOpen*/)) {
        return;
    }

    // MEMMAN NOT INITIALIZED ----------------------------------------------- //
    if (!firstBlock()) {
        static int size = (int)mm.setup.memManSize;
        static MemManFSASetup fsaSetup = mm.setup.memManFSA;
        InputInt("Init size", &size, 1024, 1024*1024);
        TextUnformatted("FSA sub-block counts");
        PushItemWidth(100);
        uint16_t fsaCountStep = 8;
        for (int fsaI = 0; fsaI < MemManFSASetup::Max; ++fsaI) {
            char const * title = mm.frameFormatStr("%d-byte sub-blocks", 1 << (fsaI+1));
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
    Text("MemMan Initialized [%s]", mm.frameByteSizeStr(size()));
    Text("%p - %p", data(), data() + size());
    // don't show shutdown if is main memory manager
    if (&mm.memMan != this) {
        if (Button("Shutdown")) {
            removeAllAllocs();
            shutdown();
        }
        Dummy(ImVec2(0.0f, 10.0f));
    }

    // TEST ALLOCATIONS ----------------------------------------------------- //
    if (CollapsingHeader("Test Allocations")) {
        if (nTestAllocs < MaxTestAllocs) {
            // ALLOCATE GENERIC
            PushItemWidth(90);
            TextUnformatted("Allocate Generic:");
            {
                static int sizeAlign[] = {4, 0};

                // create a test allocation
                if (nTestAllocs < MaxTestAllocs) {
                    InputInt2("Size/align##alloc", sizeAlign);
                    SameLine();
                    if (Button("Allocate")) {
                        void * ptr = request({.size=(size_t)sizeAlign[0], .align=(size_t)sizeAlign[1]});
                        if (ptr) {
                            addTestAlloc(ptr, "%d-byte generic", sizeAlign[0]);
                        }
                    }
                }
            }
            PopItemWidth();
            Dummy(ImVec2(0.0f, 10.0f));

            // ALLOCATE BLOCK
            PushItemWidth(90);
            TextUnformatted("Manually Create Block Object:");
            static MemBlockType selectedType = MEM_BLOCK_GOBJ;
            if (BeginCombo("###MemBlockType", memBlockTypeStr(selectedType))) {
                for (int i = MEM_BLOCK_CLAIMED; i <= MEM_BLOCK_POOL; ++i) {
                    if (Selectable(memBlockTypeStr((MemBlockType)i))) {
                        selectedType = (MemBlockType)i;
                    }
                }
                EndCombo();
            }
            PopItemWidth();

            // INPUT PARAMETERS FOR EACH BLOCK TYPE
            switch (selectedType) {
            case MEM_BLOCK_ARRAY: {
                Array_editorCreate();
                break;
            }
            case MEM_BLOCK_CHARKEYS: {
                CharKeys::editorCreate();
                break;
            }
            case MEM_BLOCK_FILE: {
                File::editorCreate();
                break;
            }
            case MEM_BLOCK_FREELIST: {
                FreeList::editorCreate();
                break;
            }
            case MEM_BLOCK_GOBJ: {
                Gobj::editorCreate();
                break;
            }
            case MEM_BLOCK_POOL: {
                Pool_editorCreate();
                break;
            }
            case MEM_BLOCK_GENERIC: {
                static int sizeAlign[] = {1024, 0};
                SameLine();
                PushItemWidth(120);
                InputInt2("Size/align##GenericBlock", sizeAlign);
                PopItemWidth();
                SameLine();
                if (Button("Create")) {
                    mm.editor.clearMemEditWindow();
                    BlockInfo * block = createBlock({
                        .size = (size_t)sizeAlign[0],
                        .align = (size_t)sizeAlign[1]
                    });
                    if (block) {
                        addTestAlloc(block->data(), "Generic block (%zu bytes)", block->dataSize());
                    }
                }
                break;
            }
            default: {}
            }
        }

        // LIST TEST ALLOCS ------------------------------------------------- //
        if (nTestAllocs) {
            Dummy(ImVec2(0.0f, 10.0f));
            TextUnformatted("Test Allocs:");

            for (int i = 0; i < nTestAllocs; ++i) {
                PushID(i);
                Text("%p (%s)", testAllocs[i].ptr, testAllocs[i].desc);
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
    }

    // list of blocks count
    Text("%zu Blocks", blockCountForDisplayOnly());

    // filter block types
    SameLine();
    TextUnformatted("Filter:");
    SameLine();
    static MemBlockType filterType = MEM_BLOCK_FILTER_ALL;
    if (BeginCombo("###MemBlockFilterType", memBlockTypeStr(filterType))) {
        for (int i = MEM_BLOCK_FREE; i <= MEM_BLOCK_FILTER_ALL; ++i) {
            if (Selectable(memBlockTypeStr((MemBlockType)i))) {
                filterType = (MemBlockType)i;
            }
        }
        EndCombo();
    }
    Separator();

    // LIST BLOCKS ---------------------------------------------------------- //
    ImGuiStyle * style = &ImGui::GetStyle();
    ImVec4 defaultTextColor = style->Colors[ImGuiCol_Text];
    int blockIndex = 0;
    for (MemMan::BlockInfo * b = firstBlock(); b; b = nextBlock(b)) {
        if (filterType != MEM_BLOCK_FILTER_ALL &&
            b->_type != filterType) {
            continue;
        }


        PushID(b);

        void * basePtr = (void *)((MemMan::BlockInfo const *)b)->basePtr();

        bool isTestAlloc = false;
        int testAllocIndex = 0;
        for (int i = 0; i < nTestAllocs; ++i) {
            if (b->data() == testAllocs[i].ptr) {
                isTestAlloc = true;
                testAllocIndex = i;
                break;
            }
        }

        // calc block name
        char const * blockName = mm.frameFormatStr("%03d (%p): %s [pad:%zu, data:%zu]",
            blockIndex,
            basePtr,
            memBlockTypeStr(b->type()),
            b->paddingSize(),
            b->dataSize()
        );

        if (b->type() == MEM_BLOCK_FREE) {
            style->Colors[ImGuiCol_Text] = ImVec4(0.6f, 0.7f, 1.0f, 1.00f);
        }
        else if (b->type() == MEM_BLOCK_CLAIMED) {
            style->Colors[ImGuiCol_Text] = ImVec4(0.9f, 0.5f, 0.5f, 1.00f);
        }
        else if (b->type() == MEM_BLOCK_GENERIC) {
            style->Colors[ImGuiCol_Text] = ImVec4(0.5f, 1.0f, 0.5f, 1.00f);
        }

        if (CollapsingHeader(blockName)) {

            style->Colors[ImGuiCol_Text] = defaultTextColor;

            // memory window is open and pointed at this block
            if (b == mm.editor.memWin.ptr) {
                if (Button("Hide Memory")) {
                    mm.editor.clearMemEditWindow();
                }
            }
            else {
                if (Button("Show Memory")) {
                    mm.editor.showMemEditWindow(blockName, b, b->blockSize());
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

            // show total block size
            SameLine();
            Text("total block size: %zu (%s)", b->blockSize(), mm.frameByteSizeStr(b->blockSize()));

            Indent();

            // sub type specifics
            switch (b->type()) {
            // ARRAY
            case MEM_BLOCK_ARRAY: {
                if (isTestAlloc) {
                    Array_editorEditBlock(*(Array<int> *)b->data());
                }
                break;
            }

            // CHARKEYS
            case MEM_BLOCK_CHARKEYS: {
                ((CharKeys *)b->data())->editorEditBlock();
                break;
            }

            // FILE
            case MEM_BLOCK_FILE: {
                ((File *)b->data())->editorEditBlock();
                break;
            }

            // FREELIST
            case MEM_BLOCK_FREELIST: {
                ((FreeList *)b->data())->editorEditBlock();
                break;
            }

            // FSA
            case MEM_BLOCK_FSA: {
                ((FSA *)b->data())->editorEditBlock();
                break;
            }

            // FSA
            case MEM_BLOCK_GOBJ: {
                ((Gobj *)b->data())->editorEditBlock();
                break;
            }

            // POOL
            case MEM_BLOCK_POOL: {
                Pool_editorEditBlock(*(Pool<int> *)b->data());
                break;
            }

            // do nothing
            default: {}
            }

            Unindent();

        }

        style->Colors[ImGuiCol_Text] = defaultTextColor;
        Separator();
        ++blockIndex;

        PopID();
    }
}

void MemMan::addTestAlloc(void * ptr, char const * formatString, ...) {
    testAllocs[nTestAllocs++] = {.ptr=ptr};

    if (formatString) {
        va_list args;
        va_start(args, formatString);
        vsnprintf(testAllocs[nTestAllocs-1].desc, TestAlloc::DescSize, formatString, args);
        va_end(args);
    }
    else {
        testAllocs[nTestAllocs-1].desc[0] = '\0';
    }
}

void MemMan::removeAlloc(uint16_t i) {
    if (i >= nTestAllocs) {
        return;
    }

    mm.memMan.request({.size=0, .ptr=testAllocs[i].ptr});

    for (int j = i + 1; j < nTestAllocs; ++j) {
        testAllocs[j-1] = testAllocs[j];
    }
    testAllocs[--nTestAllocs] = {};
}

void MemMan::removeAllAllocs() {
    while(nTestAllocs) {
        removeAlloc(nTestAllocs-1);
    }
}

#endif // DEV_INTERFACE
