#include "Editor.h"
#include <imgui.h>
#include "../MrManager.h"


using namespace ImGui;
static char temp[32];


void Editor::gui() {

    SetNextWindowPos({0, 0});
    static const ImVec2 min{250, (float)mm.WindowSize.h};
    static const ImVec2 max{mm.WindowSize.w / 2.f, min.y};
    static const float startWidth = 0;
    SetNextWindowSize({min.x+(max.x-min.x)*startWidth, min.y}, ImGuiCond_Once);
    SetNextWindowSizeConstraints(min, max);
    Begin("Editor", NULL, ImGuiWindowFlags_NoTitleBar);

    guiDemoVertColorEditor();

    guiEntityEditor();

    End();
}

void Editor::guiDemoVertColorEditor() {
    if (CollapsingHeader("Vert Color")) {
        for (int i = 0; i < 4; ++i) {
            sprintf(temp, "Vert %d", i);
            ColorPicker3(temp, mm.verts[i].getColor3(&mm.vertFloatColors[i*4]), ImGuiCond_Once|ImGuiColorEditFlags_NoInputs);
            mm.verts[i].setColor3(&mm.vertFloatColors[i*4]);
        }
    }
}

void Editor::guiEntityEditor() {
    if (CollapsingHeader("Entities", ImGuiTreeNodeFlags_DefaultOpen)) {

        static size_t justCreated;

        if (Button("Add Entity")) {
            auto e = mm.registry.create();
            justCreated = (size_t)e;
            sprintf(temp, "Entity %zu", justCreated);
            mm.registry.emplace<Info>(e, temp);
            // SetNextItemOpen(true);
        }
        else {
            justCreated = SIZE_MAX;
        }

        NewLine();

        auto view = mm.registry.view<Info>();
        view.each([this](auto const & entity, auto const & info){
            size_t eid = (size_t)entity;
            PushID(eid);
            sprintf(temp, "%s###%zu", info.name, eid);
            if (eid == justCreated) SetNextItemOpen(true);
            if (CollapsingHeader(temp)) {
                Indent();

                if (InputText("Name", (char *)&info.name[0], 16, ImGuiInputTextFlags_EnterReturnsTrue)) {
                    // // Copy default name back into buffer if empty
                    // if (strlen(info.name) == 0) {
                    //     sprintf(temp, "Entity %zu", eid);
                    //     strcpy((char *)&info.name[0], &temp[0]);
                    // }
                }
                if (eid == justCreated) SetKeyboardFocusHere(-1);
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
