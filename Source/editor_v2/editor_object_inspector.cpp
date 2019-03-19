#include "editor_object_inspector.hpp"

#include "../common/lib/imgui_wrap.hpp"
#include "../common/lib/imgui/imgui_internal.h"
#include "../common/util/log.hpp"

#include "editor.hpp"

void EditorObjectInspector::update(Editor* editor) {
    if(ImGui::Begin("Object inspector", 0, ImGuiWindowFlags_MenuBar)) {
        if(ImGui::BeginMenuBar()) {
            if(ImGui::BeginMenu("Add component...")) {
                rttr::type t = rttr::type::get<ObjectComponent>();
                auto derived_array = t.get_derived_classes();
                for(auto& d : derived_array) {
                    if(ImGui::MenuItem(d.get_name().to_string().c_str())) {
                        if(editor->getSelectedObject()) {
                            editor->getSelectedObject()->get(d);
                        }
                    }
                }   
                //imguiRecursiveDerivedMenu(scene, t);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        GameObject* so = editor->getSelectedObject();
        if(so) {
            editor->getEditorScene().objectGui(so);
            ImGui::Separator();
            ImGui::Text("Components");
            for(size_t i = 0; i < so->componentCount(); ++i) {
                auto c = so->getById(i);
                bool exists = true;
                if(ImGui::CollapsingHeader(c->get_type().get_name().to_string().c_str(), &exists, ImGuiTreeNodeFlags_DefaultOpen)) {
                    editor->getEditorScene().componentGui(c.get());
                }
                if(!exists) {
                    // TODO: Delete component
                }
                ImGui::Dummy(ImVec2(0.0f, 20.0f));
                
            }
        } else {
            ImGui::Text("Nothing selected");
        }
        ImGui::End();
    }
}