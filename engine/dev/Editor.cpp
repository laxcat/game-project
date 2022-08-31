#include "Editor.h"
#include <unordered_map>
#include <vector>
// #include <stdio.h>
#include <imgui.h>
#include <nfd.h>
#include "../MrManager.h"
#include "OriginWidget.h"
#include "../common/Path.h"
// #include "../dev/print.h"

// ImGuiTreeNodeFlags_DefaultOpen
using namespace ImGui;
static char scratchStr[32];

struct LoadedGLTF {
    std::string label;
    std::string key;
    Path path;
};
std::vector<LoadedGLTF> loadedGLTFs;

LoadedGLTF * loadedGLTFForKey(char const * key) {
    for (LoadedGLTF & item : loadedGLTFs) {
        if (item.key == key) {
            return &item;
        }
    }
    return nullptr;
}

bool setLoadedGLTFPath(char const * key, char const * path) {
    for (LoadedGLTF & item : loadedGLTFs) {
        if (item.key == key) {
            item.path.setPath(path);
            return true;
        }
    }
    return false;
}

char const * filenameForLoadedGLTF(char const * key) {
    for (LoadedGLTF & item : loadedGLTFs) {
        if (item.key == key) {
            return item.path.filenameOnly;
        }
    }
    return "";
}

void Editor::tick() {
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
    if (mm.setup.appendInsideEditor) mm.setup.appendInsideEditor();

    End();
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
        for (auto const & loadedGLTF : loadedGLTFs) {
            guiGLTF(loadedGLTF.label.c_str(), loadedGLTF.key.c_str());
        }

        if (loadedGLTFs.size() < 1000) {
            static char newLabel[64];
            PushItemWidth(200);
            InputText("GLTF Slot", newLabel, 64, ImGuiInputTextFlags_CharsNoBlank);
            PopItemWidth();
            SameLine();
            if (Button("Create New") && strlen(newLabel)) {
                std::string newKey{"editorLoaded_000"};
                static size_t nextKey = 0;
                sprintf((char *)newKey.c_str() + 13, "%03zu", nextKey++);
                loadedGLTFs.push_back({newLabel, newKey});
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
    bool keyExists = mm.rendSys.keyExists(key);

    TextUnformatted(label);

    BeginDisabled(!ready);

    char buttonLabel[100];
    sprintf(buttonLabel, "%s GLTF###loadButton", keyExists ? "Swap":"Load");
    if (Button(buttonLabel)) {
        nfdchar_t * outPath = NULL;
        nfdresult_t result = NFD_OpenDialog(NULL, ".", &outPath);

        printf("GLTF SWAP FOR KEY: %s\n", key);

        if (result == NFD_OKAY) {
            LoadedGLTF * item = loadedGLTFForKey(key);
            if (item) {
                item->path.setPath(outPath);
                mm.rendSys.createFromGLTF(item->path.full, key);
            }
            free(outPath);
        }
        else if (result == NFD_CANCEL) {
        }
        else {
            fprintf(stderr, "Error: %s\n", NFD_GetError());
        }
    }
    
    if (keyExists) {
        SameLine();
        TextUnformatted(ready ? filenameForLoadedGLTF(key) : "loading");
    }

    if (mm.rendSys.keyExists(key)){
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
