#include "Editor.h"
#include "../common/imgui_bgfx_glfw/imgui_bgfx_glfw.h"
#include "../common/imgui_bgfx_glfw/imgui_utils.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <imgui_memory_editor/imgui_memory_editor.h>
#pragma clang diagnostic pop
#include <nfd.h>
#include "../MrManager.h"
#include "../memory/CharKeys.h"
#include "../memory/mem_utils.h"
#include "../common/string_utils.h"

// ImGuiTreeNodeFlags_DefaultOpen
using namespace ImGui;

static MemoryEditor memEdit;

void Editor::init() {
    memEdit.OptShowDataPreview = true;
    // memEdit.ReadOnly = true;
    // memEdit.OptAddrDigitsCount = 4;
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
    ImVec2 sidebarSize = GetWindowSize();

    if (mm.setup.prependInsideEditor) mm.setup.prependInsideEditor();
    guiRendering();
    guiLighting();
    guiCamera();
    guiHelpers();
    // guiRenderables();
    guiGobjs();
    guiFog();
    guiColors();
    guiMem();
    #if DEBUG
    guiDebugger();
    #endif // DEBUG
    if (mm.setup.appendInsideEditor) mm.setup.appendInsideEditor();

    End();

    SetNextWindowPos(ImVec2(sidebarSize.x, 0.f), ImGuiCond_Once);
    guiWinMem();
}

void Editor::guiRendering() {
    if (CollapsingHeader("Rendering")) {
        auto user = mm.rendSys.settings.user;

        char const * items[] = {"Off", "x2", "x4", "x8", "x16"};
        int values[] = {0, 2, 4, 8, 16};
        int current;
        switch (mm.rendSys.settings.user.msaa) {
        default: { current = 0; break; }
        case  2: { current = 1; break; }
        case  4: { current = 2; break; }
        case  8: { current = 3; break; }
        case 16: { current = 4; break; }
        }
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
        if (Checkbox("Draw Scene AABB", &mm.rendSys.settings.user.drawSceneAABB) &&
            mm.rendSys.settings.user.didChangeDrawSceneAABB) {
            mm.rendSys.settings.user.didChangeDrawSceneAABB(mm.rendSys.settings.user.drawSceneAABB);
        }
        if (mm.rendSys.settings.user.drawSceneAABB) {
            SameLine();
            ColorEdit4("##aabbColor", mm.rendSys.settings.user.aabbColor, ImGuiColorEditFlags_DisplayHex);
        }
        
        if (user != mm.rendSys.settings.user) {
            mm.rendSys.settings.reinit();
        }

        if (Button("Force Reset")) {
            mm.rendSys.settings.reinit();
        }

        Dummy(ImVec2(0.0f, 20.0f));
    }    
}

void Editor::guiLighting() {
    if (CollapsingHeader("Lighting")) {

        auto & lights = mm.rendSys.lights;
        bool updateEuler = false;

        // DIRECTIONAL

        for (int i = 0; i < lights.directionalCount; ++i) {
            PushID(i);

            Text("Directional %d", i);
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

            PushID((int)id);

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
        auto oldProjType = mm.camera->projType;
        if (BeginCombo("Type", mm.camera->projTypeStr())) {
            for (int n = 0; n < mm.camera->projTypeCount(); ++n) {
                if (Selectable(mm.camera->projTypeStr(n))) {
                    mm.camera->projType = (Camera::ProjType)n;
                    mm.camera->reset();
                    // SetItemDefaultFocus();
                }
            }
            EndCombo();
        }
        bool typeChanged = (oldProjType != mm.camera->projType);

        if (Button("Reset")) {
            mm.camera->reset();
        }

        bool distChanged =  SliderFloat("Distance", &mm.camera->distance, 0.0f, 50.0f, "%.3f");
        bool targetChanged = SliderFloat3("Target", (float *)&mm.camera->target, -10.0f, 10.0f, "%.3f");

        if (mm.camera->projType == Camera::ProjType::Persp) {
            bool pitchChanged =  SliderAngle("Pitch", &mm.camera->pitch, -89.999f, 0.f, "%.3f");
            bool yawChanged =    SliderAngle("Yaw", &mm.camera->yaw);
            bool fovChanged =    SliderAngle("FOV", &mm.camera->fov, 10.f, 110.f);

            if (typeChanged || fovChanged) {
                mm.camera->updateProjection();
            }
            if (typeChanged || distChanged || pitchChanged || yawChanged || targetChanged) {
                mm.camera->updatePosFromDistancePitchYaw();
            }
        }

        else if (mm.camera->projType == Camera::ProjType::Ortho) {
            if (typeChanged || distChanged || targetChanged) {
                mm.camera->updateProjection();
            }
        }


        Dummy(ImVec2(0.0f, 20.0f));
    }
}

void Editor::guiHelpers() {
    // if (CollapsingHeader("Helpers")) {
    //     auto r = mm.rendSys.at(Renderable::OriginWidgetKey);
    //     if (r != nullptr) {
    //         Checkbox("Show Origin Widget", &r->instances[0].active);
    //         if (r->instances[0].active) {
    //             SameLine();
    //             float t = OriginWidget::scale;
    //             SliderFloat("scale##origin", &OriginWidget::scale, 1.0f, 20.0f, "%.3f");
    //             if (t != OriginWidget::scale) {
    //                 OriginWidget::updateModelFromScale();
    //             }
    //             ColorEdit3("base color", (float *)&r->materials[0].baseColor, ImGuiColorEditFlags_DisplayHex);
    //         }
    //     }
    // }
}

// void Editor::guiRenderables() {
//     if (CollapsingHeader("Renderables", ImGuiTreeNodeFlags_DefaultOpen)) {

//         static void * didCreateNew = nullptr;

//         // add new renderable
//         if (mm.rendSys.renderList->isFull() == false) {
//             static char newLabel[CharKeys::KEY_MAX];
//             static int flags = ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_EnterReturnsTrue;
//             PushItemWidth(120);
//             bool didPressEnter = InputText("New Renderable Slot", newLabel, CharKeys::KEY_MAX, flags);
//             removeKeyEnterKeyRepeat(&didPressEnter);
//             PopItemWidth();
//             // SameLine();
//             // if ((Button("Create New") || didPressEnter) && strlen(newLabel)) {
//             //     didCreateNew = mm.rendSys.create(mm.rendSys.gltfProgram, newLabel);
//             //     fixstrcpy<CharKeys::KEY_MAX>(newLabel, "");
//             // }
//             Separator();
//         }

//         // list current loaded renderables
//         for (auto n : mm.rendSys.renderList) {
//             guiRenderable((Renderable *)n->ptr, (n->ptr == didCreateNew));
//         }
//     }

// }

// void Editor::guiRenderable(Renderable * r, bool defaultOpen) {
//     int flags = 0;
//     if (defaultOpen) flags |= ImGuiTreeNodeFlags_DefaultOpen;

//     // COLLAPSING HEADER GUARDIAN STYLE
//     if (!TreeNodeEx(r->key, flags)) {
//         Separator();
//         return;
//     }

//     PushID(r);

//     bool ready = mm.rendSys.isKeySafeToDrawOrLoad(r->key);
//     bool isLoaded = r->path.isSet();
//     bool keyExistsInRender = mm.rendSys.keyExists(r->key);


//     BeginDisabled(!ready);

//     char buttonLabel[100];
//     sprintf(buttonLabel, "%s GLTF###loadButton", isLoaded ? "Swap":"Load");
//     if (Button(buttonLabel)) {
//         nfdchar_t * outPath = NULL;
//         nfdresult_t result = NFD_OpenDialog(NULL, "/Users/Shared/Dev/gltf_assets", &outPath);

//         printl("GLTF %s FOR KEY: %s", isLoaded ? "SWAP":"LOAD", r->key);

//         if (result == NFD_OKAY) {
//             mm.rendSys.createFromGLTF(outPath, r->key);
//             free(outPath);
//             printl("path after release %s", r->path);
//         }
//         else if (result == NFD_CANCEL) {
//         }
//         else {
//             fprintf(stderr, "Error: %s\n", NFD_GetError());
//         }
//     }

//     if (keyExistsInRender) {
//         // printl("keyExistsInRender %s", slot.key);
//         SameLine();
//         TextUnformatted(ready ? r->path.filename : "loading");

//         bool rotx = r->adjRotAxes[0];
//         bool roty = r->adjRotAxes[1];
//         bool rotz = r->adjRotAxes[2];
//         Checkbox("Rot.X ", &r->adjRotAxes[0]);
//         SameLine();
//         Checkbox("Rot.Y ", &r->adjRotAxes[1]);
//         SameLine();
//         Checkbox("Rot.Z ", &r->adjRotAxes[2]);
//         if (rotx != r->adjRotAxes[0] ||
//             roty != r->adjRotAxes[1] ||
//             rotz != r->adjRotAxes[2]) {
//             r->updateAdjRot();
//         }
//         if (CollapsingHeader("Instances", ImGuiTreeNodeFlags_DefaultOpen)) {
//             for (size_t i = 0; i < r->instances.size(); ++i) {
//                 PushID(i);

//                 auto & inst = r->instances[i];

//                 Checkbox("", &inst.active);
//                 SameLine();
//                 Text("Instance %zu", i);

//                 ColorEdit4("Override Color",
//                     (float *)&inst.overrideColor,
//                     ImGuiColorEditFlags_DisplayHex);

//                 float * t = (float *)&inst.model[3][0];
//                 InputFloat3("Pos", (float *)&inst.model[3][0], "%.2f", ImGuiInputTextFlags_None);
//                 PopID();
//             }
//             if (Button("Add Instance")) {
//                 r->instances.push_back({});
//             }

//         }
//         if (CollapsingHeader("Materials")) {
//             PushItemWidth(200);
//             for (size_t i = 0; i < r->materials.size(); ++i) {
//                 PushID(i);
//                 auto const & mat = r->materials[i];
//                 ColorEdit3(mat.name,
//                     (float *)&mat.baseColor,
//                     ImGuiColorEditFlags_DisplayHex);
//                 PopID();
//             }
//             PopItemWidth();
//         }
//     }
//     else {
//         BeginDisabled();
//         bool x, y, z;
//         Checkbox("Rot.X ", &x);
//         SameLine();
//         Checkbox("Rot.Y ", &y);
//         SameLine();
//         Checkbox("Rot.Z ", &z);
//         EndDisabled();
//     }
//     EndDisabled();
//     TreePop();
//     PopID();
//     Separator();
// }

void Editor::guiGobjs() {
    if (!CollapsingHeader("Game Objects", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    PushStyleColor(ImGuiCol_Header,         0xff333366);
    PushStyleColor(ImGuiCol_HeaderHovered,  0xff333388);
    PushStyleColor(ImGuiCol_HeaderActive,   0xff3333aa);

    char const * keyToSwap = nullptr;
    char const * keyToUnload = nullptr;

    for (auto node : mm.rendSys.renderList) {
        auto g = (Gobj *)node->ptr;
        TextUnformatted(node->key);
        Indent();
        if (Button("Swap")) {
            keyToSwap = node->key;
        }
        SameLine();
        if (Button("Unload")) {
            keyToUnload = node->key;
        }
        g->editorEditBlock();
        Unindent();
        Separator();
    }
    PopStyleColor(3);

    // hit swap button
    if (keyToSwap) {
        Gobj * oldGobj = mm.rendSys.gobjForKey(keyToSwap);
        char const * startPath = nullptr;
        if (oldGobj && oldGobj->loadedDirName) {
            startPath = oldGobj->loadedDirName;
        }
        nfdchar_t * outPath = NULL;
        nfdresult_t result = NFD_OpenDialog(NULL, startPath, &outPath);

        if (result == NFD_OKAY) {
            mm.createWorker([keyToSwap, outPath]{
                Gobj * newGobj = mm.memMan.createGobj(outPath, RenderSystem::typicalCountsForMinReqMat());
                Gobj * oldGobj = mm.rendSys.update(keyToSwap, newGobj);
                mm.memMan.request({.ptr=oldGobj, .size=0});
                free(outPath);
            });
        }
        else if (result == NFD_CANCEL) {}
        else {
            fprintf(stderr, "Error: %s\n", NFD_GetError());
        }
    }
    // hit unload button
    else if (keyToUnload) {
        Gobj * oldGobj = mm.rendSys.gobjForKey(keyToUnload);
        mm.rendSys.remove(keyToUnload);
        mm.memMan.request({.ptr=oldGobj, .size=0});
    }

    // add gltf with
    guiAddGobj();
}

void Editor::guiAddGobj() {
    if (!mm.rendSys.canAdd()) {
        return;
    }

    TextUnformatted("Add Gobj from glTF");

    Indent();

    static char newKeyLabel[CharKeys::KEY_MAX];
    // static int flags = ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_EnterReturnsTrue;
    static int flags = 0;
    PushItemWidth(120);
    InputText("New Gobj Key", newKeyLabel, CharKeys::KEY_MAX, flags);
    if (newKeyLabel[0] == '\0') {
        Unindent();
        return;
    }

    SameLine();
    if (Button("Load") == false) {
        Unindent();
        return;
    }

    nfdchar_t * outPath = NULL;
    nfdresult_t result = NFD_OpenDialog(NULL, ".", &outPath);
    if (result == NFD_OKAY) {
        mm.createWorker([outPath]{
            Gobj * newGobj = mm.memMan.createGobj(outPath, RenderSystem::typicalCountsForMinReqMat());
            mm.rendSys.add(newKeyLabel, newGobj);
            newKeyLabel[0] = '\0';
            free(outPath);
        });
    }
    else if (result == NFD_CANCEL) {}
    else {
        fprintf(stderr, "Error: %s\n", NFD_GetError());
    }
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
    mm.memMan.editor();
}

#if DEBUG
void Editor::guiDebugger() {
    if (CollapsingHeader("Debugger")) {
        if (Button("Break")) {
            debugBreak();
        }
        Dummy(ImVec2(0.0f, 20.0f));
    }
}
#endif // DEBUG


void Editor::guiWinMem() {
    if (memWin.ptr && memWin.open == false) clearMemEditWindow();
    if (memWin.ptr == nullptr) return;

    // only if width has been set
    if (memWin.height == 0.f) {
        memWin.height = (float)mm.windowSize.h;
    }
    if (memWin.width) {
        SetNextWindowSize(ImVec2(memWin.width, memWin.height));
    }
    Begin(memWin.title, &memWin.open, ImGuiWindowFlags_NoScrollbar);
    memEdit.DrawContents(
        memWin.ptr,
        memWin.size,
        (size_t)memWin.ptr
    );
    memWin.height = GetWindowSize().y;
    End();
    memWin.width = 0.f;

    // memEdit.DrawWindow(
    //     memWin.title,
    //     memWin.ptr,
    //     memWin.size
    // );

    // size_t basePtr = (size_t)memWin.ptr;
    // size_t roundedPtr = basePtr & (size_t)0xfffffffffffffff0;
    // size_t roundedDif = basePtr - roundedPtr;

    // memEdit.DrawWindow(
    //     memWin.title,
    //     (void *)roundedPtr,
    //     memWin.size + roundedDif,
    //     roundedPtr & 0xffff
    // );

    // TODO: enable/figure out why it wasn't working
    // memEdit.GotoAddrAndHighlight(
    //     basePtr,
    //     basePtr + memEditBlock->blockSize()
    // );
}

void Editor::showMemEditWindow(char const * title, void * ptr, size_t size) {
    memWin.open = true;
    memWin.ptr = ptr;
    memWin.size = size;
    MemoryEditor::Sizes s;
    memEdit.CalcSizes(s, size, 0x100000000);
    memWin.width = s.WindowWidth;
    snprintf(memWin.title, MemEditWindow::TitleSize, "%s###memEditWindow", title);
}

void Editor::clearMemEditWindow() {
    memWin.ptr = nullptr;
    memWin.size = 0;
    memWin.width = 0.f;
    snprintf(memWin.title, MemEditWindow::TitleSize, "");
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
//         (b->type == MEM_BLOCK_GENERIC) ? "GENERIC" :
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
// else if (b->type == MEM_BLOCK_GENERIC) {
// }

// wrote += snprintf(
//     buf + wrote,
//     bufSize - wrote,
//     "--------------------------------------------------------------------------------\n"
// );
