#include "editor_scene_inspector.hpp"

#include "../common/lib/imgui_wrap.hpp"
#include "../common/lib/imgui/imgui_internal.h"
#include "../common/util/log.hpp"

#include "editor.hpp"

static void imguiRecursiveDerivedMenu(GameScene* scene, rttr::type t) {
    auto derived_array = t.get_derived_classes();
    if(derived_array.empty()) {
        if(ImGui::MenuItem(t.get_name().to_string().c_str())) {
            scene->create(t);
        }
    } else {
        if(ImGui::BeginMenu(t.get_name().to_string().c_str())) {
            for(auto& d : derived_array) {
                imguiRecursiveDerivedMenu(scene, d);
            }
            ImGui::EndMenu();
        }
        if(ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
            scene->create(t);
            ImGui::CloseCurrentPopup();
        }
    }
}

void EditorSceneInspector::update(Editor* editor) {
    GameScene* scene = editor->getScene();

    if(ImGui::Begin("Scene Inspector", 0, ImGuiWindowFlags_MenuBar)) {
        if(ImGui::BeginMenuBar()) {
            if(ImGui::BeginMenu("Create...")) {
                rttr::type t = rttr::type::get<GameObject>();
                imguiRecursiveDerivedMenu(scene, t);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        for(unsigned i = 0; i < scene->objectCount(); ++i) {
            sceneTreeViewNode(scene->getObject(i), editor);

            
        }

        ImGui::End();
    }
}

static void guiGameObjectContextMenu(GameObject* o, Editor* editor) {
    if (ImGui::BeginPopupContextItem()) {
        if(ImGui::MenuItem("Create child")) {
            o->createChild();
        }
        if(ImGui::MenuItem("Duplicate")) {
            editor->getScene()->copyObject(o);
        }
        if(ImGui::MenuItem("Delete")) {
            editor->setSelectedObject(0);
            //scene->remove(o);
        }
        if(ImGui::MenuItem("Delete tree")) {
            editor->setSelectedObject(0);
            //scene->remove(o);
        }
        ImGui::EndPopup();
    }
} 

void EditorSceneInspector::sceneTreeViewNode(GameObject* o, Editor* editor) {
    if(o->childCount() == 0) {
        std::string type_name = o->get_derived_info().m_type.get_name().to_string();
        std::string name = MKSTR("[" << type_name << "] " << o->getName() << "##" << o);
        if(ImGui::Selectable(name.c_str(), o == editor->getSelectedObject())) {
            editor->setSelectedObject(o);
        }
        guiGameObjectContextMenu(o, editor);
        
    } else {
        bool node_open = ImGui::TreeNodeEx(
            (void*)o, 
            (editor->getSelectedObject() == o ? ImGuiTreeNodeFlags_Selected : 0) | 
            ImGuiTreeNodeFlags_OpenOnDoubleClick |
            ImGuiTreeNodeFlags_OpenOnArrow, 
            o->getName().c_str()
        );
        if(ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
            editor->setSelectedObject(o);
        }
        guiGameObjectContextMenu(o, editor);

        if(node_open) {
            for(unsigned i = 0; i < o->childCount(); ++i) {
                sceneTreeViewNode(o->getChild(i).get(), editor);
            }
            ImGui::TreePop();
        }
    }
}