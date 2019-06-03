#include "editor_object_inspector.hpp"

#include "../common/lib/imgui_wrap.hpp"
#include "../common/lib/imgui/imgui_internal.h"
#include "../common/util/log.hpp"

#include "editor.hpp"

#include "behavior/behavior.hpp"

void EditorObjectInspector::update(Editor* editor) {
    if(ImGui::Begin("Object inspector", 0, ImGuiWindowFlags_MenuBar)) {
        if(ImGui::BeginMenuBar()) {
            if(ImGui::BeginMenu("Add component...")) {
                rttr::type t = rttr::type::get<Attribute>();
                auto derived_array = t.get_derived_classes();
                for(auto& d : derived_array) {
                    if(ImGui::MenuItem(d.get_name().to_string().c_str())) {
                        if(editor->getSelectedObject()) {
                            editor->getSelectedObject()->get(d);
                            editor->backupScene("component added");
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
            so->onGui();
            
            ImGui::Separator();
            auto bhvr = so->getBehavior();
            if(ImGui::BeginCombo(
                "behavior", 
                bhvr ? bhvr->get_type().get_name().to_string().c_str() : "<null>"
            )) {
                if(ImGui::Selectable("<null>", !bhvr)) {
                    so->clearBehavior();
                    bhvr = nullptr;
                    editor->backupScene("behavior set to null");
                }
                rttr::type t = rttr::type::get<Behavior>();
                auto derived = t.get_derived_classes();
                for(auto& d : derived) {
                    if(ImGui::Selectable(
                        d.get_name().to_string().c_str(),
                        bhvr && (bhvr->get_type() == d)
                    )) {
                        so->setBehavior(d);
                        editor->backupScene("behavior set");
                    }
                }

                ImGui::EndCombo();
            }
            if(bhvr) bhvr->onGui();

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