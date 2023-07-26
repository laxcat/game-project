#include "File.h"
#include <nfd.h>
#include "../common/imgui_bgfx_glfw/imgui_bgfx_glfw.h"
#include "../MrManager.h"

using namespace ImGui;

void File::editorCreate() {
    SameLine();

    if (Button("Choose File")) {
        nfdchar_t * outPath = NULL;
        nfdresult_t result = NFD_OpenDialog(NULL, "/Users/Shared/Dev/test_assets", &outPath);

        if (result == NFD_OKAY) {
            File * file = mm.memMan.createFileHandle(outPath, false);
            free(outPath);
            if (file) {
                mm.memMan.addTestAlloc(file, "File: %s", file->_path.filename);
            }

        }
        else if (result == NFD_CANCEL) {}
        else {
            fprintf(stderr, "Error: %s\n", NFD_GetError());
        }
    }
}

void File::editorEditBlock() {
    TextUnformatted(_path.full);
    Text("%zu bytes in memory (filesize + null byte)", _size);
    char const * buttonStr = (_loaded) ? "Reload###load" : "Load###load";
    if (Button(buttonStr)) {
        load();
    }
}
