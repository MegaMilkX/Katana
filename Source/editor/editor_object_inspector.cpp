#include "editor_object_inspector.hpp"

#include "../common/lib/imgui_wrap.hpp"
#include "../common/lib/imgui/imgui_internal.h"
#include "../common/util/log.hpp"

#include "editor.hpp"

#include "../common/behavior/behavior.hpp"

void EditorObjectInspector::update(Editor* editor, const std::string& title) {
    //update(editor->getScene(), editor->getSelectedObjects(), title);
}

void EditorObjectInspector::update(GameScene* scene, ObjectSet& selected, const std::string& title) {
    if(ImGui::Begin(title.c_str(), 0, ImGuiWindowFlags_MenuBar)) {
        if(ImGui::BeginMenuBar()) {
            if(ImGui::BeginMenu("Add component...")) {
                rttr::type t = rttr::type::get<Attribute>();
                auto derived_array = t.get_derived_classes();
                for(auto& d : derived_array) {
                    if(ImGui::MenuItem(d.get_name().to_string().c_str())) {
                        for(auto& o : selected.getAll()) {
                            o->get(d);
                        }
                        // TODO: editor->backupScene("component added");
                    }
                }   
                //imguiRecursiveDerivedMenu(scene, t);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        // TODO: Handle multiple selected objects
        if(!selected.empty()) {
            GameObject* so = *selected.getAll().begin();
            so->onGui();

            ImGui::Separator();
            ImGui::Text("Components");
            for(size_t i = 0; i < so->componentCount(); ++i) {
                auto c = so->getById(i);
                bool exists = true;
                if(ImGui::CollapsingHeader(c->get_type().get_name().to_string().c_str(), &exists, ImGuiTreeNodeFlags_DefaultOpen)) {
                    c->onGui();
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