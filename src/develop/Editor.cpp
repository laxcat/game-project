#include "Editor.h"
#include <imgui.h>
#include <magic_enum.hpp>
#include "../MrManager.h"


using namespace ImGui;
static char temp[32];


void Editor::gui() {
    SetNextWindowPos({0, 0});
    static const ImVec2 min{250, (float)mm.WindowSize.h};
    static const ImVec2 max{mm.WindowSize.w / 2.f, min.y};
    static const float startWidth = .5;
    SetNextWindowSize({min.x+(max.x-min.x)*startWidth, min.y}, ImGuiCond_Once);
    SetNextWindowSizeConstraints(min, max);
    Begin("Editor", NULL, ImGuiWindowFlags_NoTitleBar);

    guiCamera();
    guiDemoVertColorEditor();

    // guiEntityEditor();

    End();
}

void Editor::guiCamera() {
    if (CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {

        constexpr auto items = magic_enum::enum_names<Camera::ControlType>();
        int current = mm.camera.controlType;
        if (BeginCombo("Control Type", items[current].data())) {
            for (int i = 0; i < items.size(); ++i) {
                if (Selectable(items[i].data())) {
                    current = i;
                    SetItemDefaultFocus();
                }
            }
            EndCombo();
        }
        mm.camera.controlType = (Camera::ControlType)current;

        if (Button("Reset")) {
            mm.camera.reset();
        }
        if (mm.camera.controlType == Camera::Orbit) {
            float fov = mm.camera.fov;
            SliderFloat("Distance", &mm.camera.distance, 0.1f, 50.0f, "%.3f");
            SliderAngle("Pitch", &mm.camera.pitch, -90.0f, 90.0f, "%.3f");
            SliderAngle("Yaw", &mm.camera.yaw);
            SliderAngle("FOV", &mm.camera.fov, 1.f, 89.f);
            SliderFloat3("Target", glm::value_ptr(mm.camera.target), -20.0f, 20.0f, "%.3f");
            if (mm.camera.fov != fov) {
                mm.camera.updateProjection();
            }
            mm.camera.updatePosFromDistancePitchYaw();
        }
        else if (mm.camera.controlType == Camera::Free) {
            Text("Click into game space to begin free camera motion. Press ESC to exit.");
            SliderFloat("Distance", &mm.camera.distance, 0.1f, 50.0f, "%.3f");
            SliderAngle("Pitch", &mm.camera.pitch, -80.0f, 80.0f, "%.3f");
            SliderAngle("Yaw", &mm.camera.yaw);
            SliderAngle("FOV", &mm.camera.fov, 1.f, 89.f);
            Checkbox("Invert Y", &mm.camera.invertFreeY);
        }

    }
    // if (CollapsingHeader("Camera (Direct)")) {
    //     SliderFloat("position x", &mm.camera.pos.x, -100.0f, 100.0f, "%.3f");
    //     SliderFloat("position y", &mm.camera.pos.y, -100.0f, 100.0f, "%.3f");
    //     SliderFloat("position z", &mm.camera.pos.z, -100.0f, 100.0f, "%.3f");
    //     SliderFloat3("target", (float *)&mm.camera.target, -20.0f, 20.0f, "%.3f");
    //     SliderFloat3("up", (float *)&mm.camera.up, -1.0f, 1.0f, "%.3f");
    //     mm.camera.update();
    // }
}

void Editor::guiDemoVertColorEditor() {
    float vertFloatColors[4*4];

    if (CollapsingHeader("Vert Color")) {
        PushItemWidth(GetWindowSize().x * 0.46f);
        int order[] = {3, 2, 0, 1};
        int i;
        for (int o = 0; o < 4; ++o) {
            i = order[o];
            sprintf(temp, "Vert %d", i);
            // get color from buffer, convert to float, store in vertFloatColors
            TestQuadSystem::getColor3(i, &vertFloatColors[i*4]);
            // use float color for ImGUI memory
            ColorPicker3(temp, &vertFloatColors[i*4], 
                ImGuiCond_Once|
                ImGuiColorEditFlags_NoLabel|
                ImGuiColorEditFlags_NoSidePreview|
                ImGuiColorEditFlags_DisplayHex
            );
            // put float color back in real buffer
            TestQuadSystem::setColor3(i, &vertFloatColors[i*4]);

            if (o % 2 == 0) SameLine();
        }
    }
}

void Editor::guiEntityEditor() {
    if (CollapsingHeader("Entities", ImGuiTreeNodeFlags_DefaultOpen)) {

        static size_t justCreatedEID;

        if (Button("Add Entity")) {
            auto e = mm.registry.create();
            justCreatedEID = (size_t)e;
            Info & newInfo = mm.registry.emplace<Info>(e);
            sprintf(newInfo.name, "Entity %zu", justCreatedEID);

            // SetNextItemOpen(true);
        }
        else {
            justCreatedEID = SIZE_MAX;
        }

        NewLine();

        auto view = mm.registry.view<Info>();
        view.each([](auto const & entity, auto const & info){
            size_t eid = (size_t)entity;
            PushID(eid);
            sprintf(temp, "%s###%zu", info.name, eid);
            if (eid == justCreatedEID) SetNextItemOpen(true);
            if (CollapsingHeader(temp)) {
                Indent();

                if (InputText("Name", (char *)&info.name[0], 16, ImGuiInputTextFlags_EnterReturnsTrue)) {
                    // // Copy default name back into buffer if empty
                    // if (strlen(info.name) == 0) {
                    //     sprintf(temp, "Entity %zu", eid);
                    //     strcpy((char *)&info.name[0], &temp[0]);
                    // }
                }
                if (eid == justCreatedEID) SetKeyboardFocusHere(-1);
        
                NewLine();
        
                if (SmallButton("Delete")) {
                    OpenPopup("Delete?");
                }
                if (BeginPopupModal("Delete?")) {
                    sprintf(temp, "Really delete entity \"%s\"?", info.name);
                    Text(temp, NULL);
                    if (Button("Cancel")) {
                        CloseCurrentPopup();
                    }
                    SameLine();
                    if (Button("Delete")) {
                        mm.registry.destroy(entity);
                        CloseCurrentPopup();
                    }
                    EndPopup();
                }

                NewLine();
                Unindent();
            }
            PopID();
        });
    }
}
