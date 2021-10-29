#include "Editor.h"
#include <imgui.h>
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

    guiDemoVertColorEditor();

    guiEntityEditor();

    End();
}

void Editor::guiDemoVertColorEditor() {
    auto testQuad = mm.rendMan.at<TestQuad>("testquad");
    float vertFloatColors[testQuad->vertCount*4];

    if (CollapsingHeader("Vert Color")) {
        PushItemWidth(GetWindowSize().x * 0.46f);
        int order[] = {3, 2, 0, 1};
        int i;
        for (int o = 0; o < 4; ++o) {
            i = order[o];
            sprintf(temp, "Vert %d", i);
            // get color from buffer, convert to float, store in vertFloatColors
            testQuad->verts[i].getColor3(&vertFloatColors[i*4]);
            // use float color for ImGUI memory
            ColorPicker3(temp, &vertFloatColors[i*4], 
                ImGuiCond_Once|
                ImGuiColorEditFlags_NoLabel|
                ImGuiColorEditFlags_NoSidePreview|
                ImGuiColorEditFlags_DisplayHex
            );
            // put float color back in real buffer
            testQuad->verts[i].setColor3(&vertFloatColors[i*4]);

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
