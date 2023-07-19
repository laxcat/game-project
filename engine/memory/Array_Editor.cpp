#include "Array.h"
#include "MemMan.h"
#include "../common/imgui_bgfx_glfw/imgui_bgfx_glfw.h"
#include "../MrManager.h"

#if DEV_INTERFACE

using namespace ImGui;

void Array_editorCreate() {
    static int max = 10;
    SameLine();
    PushItemWidth(120);
    InputInt("Max##ArrayBlock", &max);
    SameLine();
    if (Button("Create")) {
        mm.editor.clearMemEditWindow();
        Array<int> * arr = mm.memMan.createArray<int>((size_t)max);
        if (arr) {
            mm.memMan.addTestAlloc(arr, "Array<int> block (%d-item max)", max);
        }
    }
}

void Array_editorEditBlock(Array<int> & arr) {
    // auto b = (MemMan::BlockInfo *)block;
    ImGuiStyle * style = &ImGui::GetStyle();

    // auto & arr = *(Array<int> *)b->data();

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
    EndTable();
}

#endif // DEV_INTERFACE
