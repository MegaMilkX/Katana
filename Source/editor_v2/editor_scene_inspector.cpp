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

static GameObject* guiGameObjectContextMenu(GameObject* o, Editor* editor) {
    if (ImGui::BeginPopupContextItem()) {
        if(ImGui::MenuItem("Create child")) {
            o->createChild();
        }
        if(o->getParent() != 0) {
            ImGui::Separator();
            if(ImGui::MenuItem("Move to top")) {
                o->getScene()->copyObject(o);
                if(editor->getSelectedObject() == o) editor->setSelectedObject(0);
                if(o->getParent() == 0) {
                    o->getScene()->removeRecursive(o);
                } else {
                    o->getParent()->removeChildRecursive(o);
                }
                o = 0;
            }
        }
        ImGui::Separator();
        if(ImGui::MenuItem("Duplicate")) {
            editor->getScene()->copyObject(o);
        }
        ImGui::Separator();
        if(ImGui::MenuItem("Delete")) {
            if(editor->getSelectedObject() == o)editor->setSelectedObject(0);
            if(o->getParent() == 0) {
                o->getScene()->remove(o);
            } else {
                o->getParent()->removeChild(o);
            }
            o = 0;
        }
        if(ImGui::MenuItem("Delete tree")) {
            if(editor->getSelectedObject() == o)editor->setSelectedObject(0);
            if(o->getParent() == 0) {
                o->getScene()->removeRecursive(o);
            } else {
                o->getParent()->removeChildRecursive(o);
            }
            o = 0;
        }
        ImGui::EndPopup();
    }
    return o;
} 

void EditorSceneInspector::sceneTreeViewNode(GameObject* o, Editor* editor) {
    if(o->childCount() == 0) {
        std::string type_name = o->get_derived_info().m_type.get_name().to_string();
        std::string name = MKSTR("[" << type_name << "] " << o->getName() << "##" << o);
        ImGui::PushID(name.c_str());
        if(ImGui::Selectable(name.c_str(), o == editor->getSelectedObject())) {
            editor->setSelectedObject(o);
        }
        if(ImGui::BeginDragDropSource(0)) {
            ImGui::SetDragDropPayload("DND_OBJECT", &o, sizeof(o));
            ImGui::Text(name.c_str());
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_OBJECT")) {
                GameObject* tgt_dnd_so = *(GameObject**)payload->Data;
                auto clone = o->createClone(tgt_dnd_so);
                if(tgt_dnd_so->getParent() == 0) {
                    tgt_dnd_so->getScene()->removeRecursive(tgt_dnd_so);
                } else {
                    tgt_dnd_so->getParent()->removeChildRecursive(tgt_dnd_so);
                }

                editor->setSelectedObject(clone.get());
            }
            ImGui::EndDragDropTarget();
        }
        guiGameObjectContextMenu(o, editor);
        ImGui::PopID();
    } else {
        ImGui::PushID(o);
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
        if(ImGui::BeginDragDropSource(0)) {
            ImGui::SetDragDropPayload("DND_OBJECT", &o, sizeof(o));
            ImGui::Text(o->getName().c_str());
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_OBJECT")) {
                GameObject* tgt_dnd_so = *(GameObject**)payload->Data;
                auto clone = o->createClone(tgt_dnd_so);
                if(tgt_dnd_so->getParent() == 0) {
                    tgt_dnd_so->getScene()->removeRecursive(tgt_dnd_so);
                } else {
                    tgt_dnd_so->getParent()->removeChildRecursive(tgt_dnd_so);
                }
                editor->setSelectedObject(clone.get());
            }
            ImGui::EndDragDropTarget();
        }
        o = guiGameObjectContextMenu(o, editor);
        ImGui::PopID();

        if(node_open) {
            if(o) {
                for(unsigned i = 0; i < o->childCount(); ++i) {
                    sceneTreeViewNode(o->getChild(i).get(), editor);
                }
            }
            ImGui::TreePop();
        }
    }
}