#include "editor_object_inspector.hpp"

#include "../common/lib/imgui_wrap.hpp"
#include "../common/lib/imgui/imgui_internal.h"
#include "../common/util/log.hpp"

#include "editor.hpp"

void EditorObjectInspector::update(Editor* editor, const std::string& title) {
    //update(editor->getScene(), editor->getSelectedObjects(), title);
}

void EditorObjectInspector::update(GameScene* scene, ObjectSet& selected, const std::string& title) {
    if(ImGui::Begin(title.c_str(), 0, ImGuiWindowFlags_MenuBar)) {
        if(!selected.empty()) {
            if(ImGui::BeginMenuBar()) {
                if(ImGui::BeginMenu("Add attribute...")) {
                    auto& table = getAttribTypeLib().getTable();
                    for(auto& kv : table) {
                        if(ImGui::BeginMenu(kv.first.c_str())) {
                            for(auto t : kv.second) {
                                if(ImGui::MenuItem(t.get_name().to_string().c_str())) {
                                    for(auto& o : selected.getAll()) {
                                        o->get(t);
                                        // TODO: editor->backupScene("component added");
                                    }
                                }
                            }
                            ImGui::EndMenu();
                        }
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }
        }

        // TODO: Handle multiple selected objects
        if(!selected.empty()) {
            ktNode* so = *selected.getAll().begin();
            so->onGui();

            if(so->componentCount()) {
                ImGui::Separator();
                ImGui::Text("Attributes");
            }
            for(size_t i = 0; i < so->componentCount(); ++i) {
                auto c = so->getById(i);
                bool exists = true;
                if(ImGui::CollapsingHeader(MKSTR(c->getIconCode() << " " << c->get_type().get_name().to_string()).c_str(), &exists, ImGuiTreeNodeFlags_DefaultOpen)) {
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
    }
    ImGui::End();
}