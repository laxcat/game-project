#include "Editor.h"
#include <unordered_map>
#include <vector>
// #include <stdio.h>
#include "../common/imgui_bgfx_glfw/imgui_bgfx_glfw.h"
#include <imgui_memory_editor/imgui_memory_editor.h>
#include <nfd.h>
#include "../MrManager.h"
#include "OriginWidget.h"
#include "../common/Path.h"
#include "../memory/mem_utils.h"
// #include "../dev/print.h"

// ImGuiTreeNodeFlags_DefaultOpen
using namespace ImGui;
static char scratchStr[32];

static MemoryEditor memEdit;

// used for guiWinMem
static void * memEditPtr = nullptr;
static size_t memEditSz  = 0;
static char memEditTitle[128];

// used for guiWinMem2 (better, newer)
static MemMan2::BlockInfo const * memEditBlock = nullptr;


struct GLTFSlot {
    std::string label;
    std::string key;
    Path path;
};
std::vector<GLTFSlot> gltfSlots;

static GLTFSlot * gltfSlotForKey(char const * key) {
    for (GLTFSlot & slot : gltfSlots) {
        if (slot.key == key) {
            return &slot;
        }
    }
    return nullptr;
}



// bool setGLTFSlotPath(char const * key, char const * path) {
//     for (GLTFSlot & slot : gltfSlots) {
//         if (slot.key == key) {
//             slot.path.setPath(path);
//             return true;
//         }
//     }
//     return false;
// }

static char const * filenameForLoadedGLTF(char const * key) {
    for (GLTFSlot & slot : gltfSlots) {
        if (slot.key == key) {
            return slot.path.filenameOnly;
        }
    }
    return "";
}

static char const * byteSizeStr(size_t byteSize) {
    // " %.0f MB"
    char * ret = mm.tempStr(16);
    if (byteSize > 1024*1024) {
        snprintf(ret, 16, "%.0f MB", round((double)byteSize/(1024*1024)));
    }
    else if (byteSize > 1024) {
        snprintf(ret, 16, "%.0f KB", round((double)byteSize/1024));
    }
    else {
        snprintf(ret, 16, "%.0f bytes", round((double)byteSize));
    }
    return (char const *)ret;
}

Editor::Editor() {
    memEdit.OptShowDataPreview = true;
    // memEdit.ReadOnly = true;
    memEdit.OptAddrDigitsCount = 4;
}

void Editor::tick() {
    if (!GetCurrentContext()) return;

    SetNextWindowPos({0, 0});
    ImVec2 min{250, (float)mm.windowSize.h};
    ImVec2 max{mm.windowSize.w / 2.f, min.y};
    float startWidth = .3;
    SetNextWindowSize({min.x+(max.x-min.x)*startWidth, min.y}, ImGuiCond_Once);
    SetNextWindowSizeConstraints(min, max);
    Begin("Editor", NULL, ImGuiWindowFlags_NoTitleBar);

    if (mm.setup.prependInsideEditor) mm.setup.prependInsideEditor();
    guiRendering();
    guiLighting();
    guiCamera();
    guiHelpers();
    guiGLTFs();
    guiFog();
    guiColors();
    guiMem();
    guiMem2();
    if (mm.setup.appendInsideEditor) mm.setup.appendInsideEditor();

    End();

    guiWinMem2();
}

void Editor::guiRendering() {
    if (CollapsingHeader("Rendering")) {
        auto user = mm.rendSys.settings.user;

        char const * items[] = {"Off", "x4", "x8", "x16"};
        std::unordered_map<int, int> map{{0,0},{4,1},{8,2},{16,3}};
        int values[] = {0, 4, 8, 16};
        int current = map[mm.rendSys.settings.user.msaa];
        char const * item = items[current];
        PushItemWidth(60);
        if (BeginCombo("MSAA", item)) {
            for (int n = 0; n < IM_ARRAYSIZE(items); ++n) {
                if (Selectable(items[n])) {
                    current = n;
                    SetItemDefaultFocus();
                }
            }
            EndCombo();
        }
        PopItemWidth();
        mm.rendSys.settings.user.msaa = values[current];

        Checkbox("V-Sync", &mm.rendSys.settings.user.vsync);
        Checkbox("Max Anisotropy", &mm.rendSys.settings.user.maxAnisotropy);
        
        if (user != mm.rendSys.settings.user) mm.rendSys.settings.reinit();

        Dummy(ImVec2(0.0f, 20.0f));
    }    
}

void Editor::guiLighting() {
    if (CollapsingHeader("Lighting")) {

        auto & lights = mm.rendSys.lights;
        bool updateEuler = false;

        // DIRECTIONAL

        for (size_t i = 0; i < lights.directionalCount; ++i) {
            sprintf(scratchStr, "dirlight%zu", i);
            PushID(scratchStr);

            Text("Directional %zu", i);
            Text("Direction");
            PushItemWidth(GetWindowWidth() * 0.4f);
            auto euler = lights.dirDataDirAsEuler[i];
            SliderAngle("X", &lights.dirDataDirAsEuler[i].x, -90.0f, 90.0f, "%.3f");
            SameLine();
            SliderAngle("Z", &lights.dirDataDirAsEuler[i].y, -90.0f, 90.0f, "%.3f");
            if (euler != lights.dirDataDirAsEuler[i]) {
                updateEuler = true;
            }
            PopItemWidth();

            Text("Strength");
            SliderFloat("Ambient", &lights.dirStrengthAmbientAt(i), 0.0f, 1.0f, "%.3f");
            SliderFloat("Diffuse", &lights.dirStrengthDiffuseAt(i), 0.0f, 1.0f, "%.3f");
            SliderFloat("Specular", &lights.dirStrengthSpecularAt(i), 0.0f, 1.0f, "%.3f");
            SliderFloat("Strength", &lights.dirStrengthOverallAt(i), 0.0f, 1.0f, "%.3f");

            ColorEdit3("Color", (float *)&lights.dirDataColor[i], ImGuiColorEditFlags_DisplayHex);
            
            Separator();

            PopID();
        }
        if (updateEuler) {
            lights.updateGlobalFromEuler();
            // GlobalLightWidget::updateModel();
        }

        if (Button("Add Directional")) {
            lights.addDirectional();
        }

        // POINT
        for (auto const & idIndexPair : mm.rendSys.lights.pointIndexById) {
            size_t id = idIndexPair.first;
            size_t i = idIndexPair.second;

            sprintf(scratchStr, "pointlight%zu", id);
            PushID(scratchStr);

            Text("Point %zu", id);
            SliderFloat3("Pos", (float *)&lights.pointPosAt(i), -10.0f, 10.0f, "%.3f");
            SliderFloat("Radius", &lights.pointRadiusAt(i), 0.5f, 10.0f, "%.3f");

            Text("Strength");
            SliderFloat("Ambient", &lights.pointStrengthAmbientAt(i), 0.0f, 1.0f, "%.3f");
            SliderFloat("Diffuse", &lights.pointStrengthDiffuseAt(i), 0.0f, 1.0f, "%.3f");
            SliderFloat("Specular", &lights.pointStrengthSpecularAt(i), 0.0f, 1.0f, "%.3f");
            SliderFloat("Strength", &lights.pointStrengthOverallAt(i), 0.0f, 1.0f, "%.3f");

            ColorEdit3("Color", (float *)&lights.pointDataColor[i], ImGuiColorEditFlags_DisplayHex);

            if (Button("Remove")) {
                lights.removePoint(i);
            }
            
            Separator();

            PopID();
        }

        if (Button("Add Point")) {
            lights.addPoint();
        }

        if (lights.pointCount) {
            SliderFloat("Point Light Cutoff", &lights.pointAttenCutoff, 0.00001f, 0.99999f, "%.5f");
        }

        Dummy(ImVec2(0.0f, 20.0f));
    }
}

void Editor::guiCamera() {
    if (CollapsingHeader("Camera (Orbit Target)")) {
        auto oldProjType = mm.camera.projType;
        if (BeginCombo("Type", mm.camera.projTypeStr())) {
            for (int n = 0; n < mm.camera.projTypeCount(); ++n) {
                if (Selectable(mm.camera.projTypeStr(n))) {
                    mm.camera.projType = (Camera::ProjType)n;
                    mm.camera.reset();
                    // SetItemDefaultFocus();
                }
            }
            EndCombo();
        }
        bool typeChanged = (oldProjType != mm.camera.projType);

        if (Button("Reset")) {
            mm.camera.reset();
        }

        bool distChanged =  SliderFloat("Distance", &mm.camera.distance, 0.0f, 50.0f, "%.3f");
        bool targetChanged = SliderFloat3("Target", (float *)&mm.camera.target, -10.0f, 10.0f, "%.3f");

        if (mm.camera.projType == Camera::ProjType::Persp) {
            bool pitchChanged =  SliderAngle("Pitch", &mm.camera.pitch, -89.999f, 0.f, "%.3f");
            bool yawChanged =    SliderAngle("Yaw", &mm.camera.yaw);
            bool fovChanged =    SliderAngle("FOV", &mm.camera.fov, 1.f, 89.f);

            if (typeChanged || fovChanged) {
                mm.camera.updateProjection();
            }
            if (typeChanged || distChanged || pitchChanged || yawChanged || targetChanged) {
                mm.camera.updatePosFromDistancePitchYaw();
            }
        }

        else if (mm.camera.projType == Camera::ProjType::Ortho) {
            if (typeChanged || distChanged || targetChanged) {
                mm.camera.updateProjection();
            }
        }


        Dummy(ImVec2(0.0f, 20.0f));
    }
}

void Editor::guiHelpers() {
    if (CollapsingHeader("Helpers")) {
        try { 
            auto r = mm.rendSys.at(Renderable::OriginWidgetKey);
            Checkbox("Show Origin Widget", &r->instances[0].active);
            if (r->instances[0].active) {
                SameLine();
                float t = OriginWidget::scale;
                SliderFloat("scale##origin", &OriginWidget::scale, 1.0f, 20.0f, "%.3f");
                if (t != OriginWidget::scale) {
                    OriginWidget::updateModelFromScale();
                }
                ColorEdit3("base color", (float *)&r->materials[0].baseColor, ImGuiColorEditFlags_DisplayHex);
            }
        } catch (...) {}
        // try { 
        //     auto r = mm.rendSys.at(Renderable::GlobalLightWidgetKey);
        //     Checkbox("Show Global Light Widget", &r->active);
        //     if (r->active) {
        //         float scale = GlobalLightWidget::scale;
        //         glm::vec3 pos = GlobalLightWidget::pos;
        //         SliderFloat3("position", (float *)&GlobalLightWidget::pos, -20.0f, 20.0f, "%.3f");
        //         SliderFloat("scale##globallight", &GlobalLightWidget::scale, 1.0f, 20.0f, "%.3f");
        //         if (scale != GlobalLightWidget::scale || pos != GlobalLightWidget::pos) {
        //             GlobalLightWidget::updateModel();
        //         }
        //     }
        // } catch (...) {}
        
        // wireframe moved to "debug state"
        // auto temp = bgfxDebug.capture();
        // Checkbox("Show Wireframe", &bgfxDebug.wireframe);
        // bgfxDebug.update(temp);
        // Dummy(ImVec2(0.0f, 20.0f));
    }    
}

void Editor::guiGLTFs() {
    if (CollapsingHeader("GLTFs")) {
        for (auto const & gltfSlot : gltfSlots) {
            guiGLTF(gltfSlot.label.c_str(), gltfSlot.key.c_str());
        }

        if (gltfSlots.size() < 1000) {
            static char newLabel[64];
            PushItemWidth(200);
            InputText("GLTF Slot", newLabel, 64, ImGuiInputTextFlags_CharsNoBlank);
            PopItemWidth();
            SameLine();
            if (Button("Create New") && strlen(newLabel)) {
                std::string newKey{"editorLoaded_000"};
                static size_t nextKey = 0;
                sprintf((char *)newKey.c_str() + 13, "%03zu", nextKey++);
                gltfSlots.push_back({newLabel, newKey});
                newLabel[0] = '\0';
            }
        }

        Dummy(ImVec2(0.0f, 20.0f));
    }

}

void Editor::guiGLTF(char const * label, char const * key) {
    // print("guiGLTF KEY (%p) %s\n", key, key);

    PushID(key);

    bool ready = mm.rendSys.isKeySafeToDrawOrLoad(key);
    bool keyExistsInRender = mm.rendSys.keyExists(key);

    TextUnformatted(label);

    BeginDisabled(!ready);

    char buttonLabel[100];
    sprintf(buttonLabel, "%s GLTF###loadButton", keyExistsInRender ? "Swap":"Load");
    if (Button(buttonLabel)) {
        nfdchar_t * outPath = NULL;
        nfdresult_t result = NFD_OpenDialog(NULL, "/Users/Shared/Dev/gltf_assets", &outPath);

        printl("GLTF %s FOR KEY: %s", keyExistsInRender ? "SWAP":"LOAD", key);

        if (result == NFD_OKAY) {
            GLTFSlot * slot = gltfSlotForKey(key);
            printl("slot %p", slot);
            if (slot) {
                slot->path.setPath(outPath);
                mm.rendSys.createFromGLTF(slot->path.full, key);
            }
            free(outPath);
        }
        else if (result == NFD_CANCEL) {
        }
        else {
            fprintf(stderr, "Error: %s\n", NFD_GetError());
        }
    }
    
    if (keyExistsInRender) {
        // printl("keyExistsInRender %s", key);
        SameLine();
        TextUnformatted(ready ? filenameForLoadedGLTF(key) : "loading");

        auto r = mm.rendSys.at(key);
        bool rotx = r->adjRotAxes[0];
        bool roty = r->adjRotAxes[1];
        bool rotz = r->adjRotAxes[2];
        Checkbox("Rot.X ", &r->adjRotAxes[0]);
        SameLine();
        Checkbox("Rot.Y ", &r->adjRotAxes[1]);
        SameLine();
        Checkbox("Rot.Z ", &r->adjRotAxes[2]);
        if (rotx != r->adjRotAxes[0] ||
            roty != r->adjRotAxes[1] ||
            rotz != r->adjRotAxes[2]) {
            r->updateAdjRot();
        }
        if (CollapsingHeader("Instances", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (size_t i = 0; i < r->instances.size(); ++i) {
                PushID(i);

                auto & inst = r->instances[i];

                Checkbox("", &inst.active);
                SameLine();
                Text("Instance %zu", i);

                ColorEdit4("Override Color",
                    (float *)&inst.overrideColor,
                    ImGuiColorEditFlags_DisplayHex);

                float * t = (float *)&inst.model[3][0];
                InputFloat3("Pos", (float *)&inst.model[3][0], "%.2f", ImGuiInputTextFlags_None);
                PopID();
            }
            if (Button("Add Instance")) {
                r->instances.push_back({});
            }

        }
        if (CollapsingHeader("Materials")) {
            PushItemWidth(200);
            for (size_t i = 0; i < r->materials.size(); ++i) {
                PushID(i);
                auto const & mat = r->materials[i];
                ColorEdit3(mat.name,
                    (float *)&mat.baseColor,
                    ImGuiColorEditFlags_DisplayHex);
                PopID();
            }
            PopItemWidth();
        }
    }
    else {
        BeginDisabled();
        bool x, y, z;
        Checkbox("Rot.X ", &x);
        SameLine();
        Checkbox("Rot.Y ", &y);
        SameLine();
        Checkbox("Rot.Z ", &z);
        EndDisabled();
    }
    EndDisabled();
    Separator();

    PopID();
}

void Editor::guiFog() {
    if (CollapsingHeader("Fog (Distance Clipping)")) {
        SliderFloat("min distance", &mm.rendSys.fog.minDistance, 1.0f, 150.0f, "%.3f");
        SliderFloat("fade distance", &mm.rendSys.fog.fadeDistance, 0.0f, 100.0f, "%.3f");
        SliderFloat("amount", &mm.rendSys.fog.amount, 0.0f, 1.0f, "%.3f");

        Dummy(ImVec2(0.0f, 20.0f));
    }
}

void Editor::guiColors() {
    if (CollapsingHeader("Colors")) {
        ColorEdit4("background", (float *)&mm.rendSys.colors.background.data, ImGuiColorEditFlags_DisplayHex);

        Dummy(ImVec2(0.0f, 20.0f));
    }
}

void Editor::guiMem() {
    if (CollapsingHeader("Memory")) {
        Text("Total: %s", byteSizeStr(mm.memMan.size()));

        Indent();

        if (Button("Print Info")) {
            mm.memMan.printInfo();
            // printl("wut.");
        }
        SameLine();
        if (Button("Check Blocks")) {
            mm.memMan.checkAllBlocks();
        }

        ImGuiStyle * style = &ImGui::GetStyle();
        ImVec4 defaultTextColor = style->Colors[ImGuiCol_Text];

        int i = 0;
        for (MemMan::Block const * b = mm.memMan.firstBlock(); b; b = mm.memMan.nextBlock(*b)) {
            // calc block name
            char * str = mm.tempStr(128);
            snprintf(str, 128, "%03d: %s Block (%s)",
                i,
                memBlockTypeStr(b->type()),
                byteSizeStr(b->dataSize())
                );
            bool isSelected = ((void *)b == (void *)memEditPtr);
            if (b->type() == MEM_BLOCK_FREE) {
                style->Colors[ImGuiCol_Text] = ImVec4(0.6f, 0.7f, 1.0f, 1.00f);
            }
            else if (b->type() == MEM_BLOCK_CLAIMED) {
                style->Colors[ImGuiCol_Text] = ImVec4(0.9f, 0.5f, 0.5f, 1.00f);
            }
            if (Selectable(str, isSelected)) {
                if (isSelected) {
                    clearMemEditWindow();
                }
                else {
                    memEdit.Open = true;
                    memEditPtr = (void *)b;
                    memEditSz = b->totalSize();
                    snprintf(memEditTitle, 64, "%s###memEditWindow", str);
                }
            }
            style->Colors[ImGuiCol_Text] = defaultTextColor;
            Separator();
            ++i;
        }

        Unindent();
    }
}


void Editor::guiMem2() {
    if (CollapsingHeader("Mem 2", ImGuiTreeNodeFlags_DefaultOpen)) {
        MemMan2 & memMan = mm.memMan2;

        // not initialized
        if (!memMan.firstBlock()) {
            static int size = 1024*1024;
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
                memMan.init(setup);
            }
        }
        // initialized
        else {
            Text("%p - %p", memMan.data(), memMan.data() + memMan.size());
            SameLine();
            if (Button("Shutdown")) {
                memMan.shutdown();
            }

            static MemBlockType selectedType = MEM_BLOCK_CLAIMED;

            PushItemWidth(90);
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
                InputInt2("Size/align", sizeAlign);
                SameLine();
                if (Button("Claim")) {
                    clearMemEditWindow();
                    memMan.request(sizeAlign[0], sizeAlign[1]);
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
            }
            }

            PopItemWidth();

            Separator();
        }

        ImGuiStyle * style = &ImGui::GetStyle();
        ImVec4 defaultTextColor = style->Colors[ImGuiCol_Text];
        int blockIndex = 0;
        for (MemMan2::BlockInfo * b = memMan.firstBlock(); b; b = memMan.nextBlock(b)) {
            PushID(blockIndex);

            void * basePtr = (void *)((MemMan2::BlockInfo const *)b)->basePtr();

            // calc block name
            char * str = mm.tempStr(128);
            snprintf(str, 128, "%03d (0x%06X): %s Block [%s][%s]",
                blockIndex,
                (uint32_t)((size_t)basePtr & 0xffffff),
                memBlockTypeStr(b->type()),
                byteSizeStr(b->paddingSize()),
                byteSizeStr(b->dataSize())
            );

            bool isSelected = (b == memEditBlock);
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
                    clearMemEditWindow();
                }
                else {
                    memEdit.Open = true;
                    memEditBlock = b;
                    snprintf(memEditTitle, 128, "%s###memEditWindow", str);
                }
            }

            // show free button
            if (b->type() != MEM_BLOCK_FREE && b->type() != MEM_BLOCK_FSA) {
                SameLine();
                if (Button("Free")) {
                    clearMemEditWindow();
                    b = memMan.release(b);
                }
            }

            // sub type specifics
            // FSA
            if (b->type() == MEM_BLOCK_FSA) {
                FSA * fsa = (FSA *)b->data();
                Indent();
                for (uint16_t fsaGroup = 0; fsaGroup < FSA::Max; ++fsaGroup) {
                    uint16_t nSubBlocks = fsa->subBlockCountForIndex(fsaGroup);
                    if (nSubBlocks == 0) continue;
                    PushID(fsaGroup);
                    char * titleStr = mm.tempStr(64);
                    snprintf(titleStr, 64, "%5d %d-byte sub-blocks", nSubBlocks, 1 << (fsaGroup + 1));
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
                                    uint32_t color = fsa->isFree(fsaGroup, subBlockIndex) ? 0xff888888 : 0xff333333;
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
}


void Editor::guiWinMem() {
    if (memEditPtr && memEdit.Open == false) clearMemEditWindow();
    if (memEditPtr == nullptr) return;
    memEdit.DrawWindow(memEditTitle, memEditPtr, memEditSz);
}

void Editor::guiWinMem2() {
    if (memEditBlock && memEdit.Open == false) clearMemEditWindow();
    if (memEditBlock == nullptr) return;
    size_t basePtr = (size_t)memEditBlock->basePtr();
    size_t memEditPtrRounded = basePtr & (size_t)0xfffffffffffffff0;
    size_t memEditRoundedDif = basePtr - memEditPtrRounded;
    // printl("ptr %p, rounded: %p, (diff=%zu)", memEditPtr, (void *)memEditPtrRounded, memEditRoundedDif);
    memEdit.DrawWindow(
        memEditTitle,
        (void *)memEditPtrRounded,
        memEditBlock->blockSize() + memEditRoundedDif,
        memEditPtrRounded & 0xffff
    );
    // memEdit.DrawWindow(memEditTitle, memEditPtr, memEditSz, (size_t)memEditPtr);
    memEdit.GotoAddrAndHighlight(
        basePtr,
        basePtr + memEditBlock->blockSize());
}

void Editor::clearMemEditWindow() {
    memEditPtr = nullptr;
    memEditSz = 0;
    memEditBlock = nullptr;
    snprintf(memEditTitle, 64, "");
}























// wrote += snprintf(
//     buf + wrote,
//     bufSize - wrote,
//     "--------------------------------------------------------------------------------\n"
//     "Block %d, %s\n"
//     "--------------------\n"
//     "Base: %p, Data: %p, BlockDataSize: %zu\n"
//     ,
//     i,
//         (b->type == MEM_BLOCK_FREE)     ? "FREE"  :
//         (b->type == MEM_BLOCK_POOL)     ? "POOL"  :
//         (b->type == MEM_BLOCK_STACK)    ? "STACK" :
//         (b->type == MEM_BLOCK_FILE)     ? "FILE" :
//         (b->type == MEM_BLOCK_EXTERNAL) ? "EXTERNAL" :
//         "(unknown type)"
//     ,
//     b,
//     b->data(),
//     b->dataSize()
// );

// if (b->type == MEM_BLOCK_FREE) {
// }
// else if (b->type == MEM_BLOCK_POOL) {
//     Pool * pool = (Pool *)b->data();
//     wrote += snprintf(
//         buf + wrote,
//         bufSize - wrote,
//         "PoolInfoSize: %zu, ObjSize: %zu, MaxCount/DataSize: *%zu=%zu, FirstFree: %zu\n"
//         ,
//         sizeof(Pool),
//         pool->objSize(),
//         pool->objMaxCount(),
//         pool->size(),
//         pool->freeIndex()
//     );
// }
// else if (b->type == MEM_BLOCK_STACK) {
//     Stack * stack = (Stack *)b->data();
//     wrote += snprintf(
//         buf + wrote,
//         bufSize - wrote,
//         "StackInfoSize: %zu, DataSize: %zu, Head: %zu\n"
//         ,
//         sizeof(Stack),
//         stack->size(),
//         stack->head()
//     );
// }
// else if (b->type == MEM_BLOCK_FILE) {
//     File * file = (File *)b->data();
//     wrote += snprintf(
//         buf + wrote,
//         bufSize - wrote,
//         "FileInfoSize: %zu, FileSize: %zu, DataSize: %zu, Head: %zu, Loaded: %s\n"
//         "Path: %s\n"
//         ,
//         sizeof(File),
//         file->fileSize(),
//         file->size(),
//         file->head(),
//         file->loaded()?"yes":"no",
//         file->path()
//     );
// }
// else if (b->type == MEM_BLOCK_EXTERNAL) {
// }

// wrote += snprintf(
//     buf + wrote,
//     bufSize - wrote,
//     "--------------------------------------------------------------------------------\n"
// );
