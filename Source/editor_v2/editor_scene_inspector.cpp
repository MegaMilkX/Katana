#include "editor_scene_inspector.hpp"

#include "../common/lib/imgui_wrap.hpp"
#include "../common/lib/imgui/imgui_internal.h"
#include "../common/util/log.hpp"

#include "editor.hpp"

#include "scene/third_person_camera.hpp"

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

static GameObject* guiGameObjectContextMenu(GameObject* o, Editor* editor) {
    if (ImGui::BeginPopupContextItem()) {
        if(ImGui::BeginMenu("Create child...")) {
            rttr::type t = rttr::type::get<GameObject>();
            auto derived_array = t.get_derived_classes();
            for(auto& d : derived_array) {
                if(ImGui::MenuItem(d.get_name().to_string().c_str())) {
                    editor->setSelectedObject(o->createChild(d).get());
                }
            }
            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Replace with...")) {
            rttr::type t = rttr::type::get<GameObject>();
            auto derived_array = t.get_derived_classes();
            for(auto& d : derived_array) {
                if(ImGui::MenuItem(d.get_name().to_string().c_str())) {
                    if(o->getParent() == 0) {
                        auto new_o = o->getScene()->create(d);
                        new_o->copyRecursive(o);
                        new_o->copyComponentsRecursive(o);
                        o->getScene()->removeRecursive(o);
                        editor->setSelectedObject(new_o);
                        new_o->refreshAabb();
                    } else {
                        auto new_o = o->getParent()->createChild(d);
                        new_o->copyRecursive(o);
                        new_o->copyComponentsRecursive(o);
                        o->getParent()->removeChildRecursive(o);
                        editor->setSelectedObject(new_o.get());
                        new_o->refreshAabb();
                    }
                    o = 0;
                }
            }
            ImGui::EndMenu();
        }
        if(o && (o->getParent() != 0)) {
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
            editor->setSelectedObject(editor->getScene()->copyObject(o));
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

void EditorSceneInspector::update(Editor* editor) {
    GameScene* scene = editor->getScene();

    if(ImGui::Begin("Scene Inspector", 0, ImGuiWindowFlags_MenuBar)) {
        if(ImGui::BeginMenuBar()) {
            if(ImGui::BeginMenu("Create...")) {
                rttr::type t = rttr::type::get<GameObject>();
                auto derived_array = t.get_derived_classes();
                for(auto& d : derived_array) {
                    if(ImGui::MenuItem(d.get_name().to_string().c_str())) {
                        editor->setSelectedObject(editor->getScene()->create(d));
                    }
                }   
                //imguiRecursiveDerivedMenu(scene, t);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        if(ImGui::InputText("Find", search_string_buf, 256)) {

        }
        std::string find_str = search_string_buf;
        if(find_str.empty()) {
            for(unsigned i = 0; i < scene->objectCount(); ++i) {
                sceneTreeViewNode(scene->getObject(i), editor);            
            }
        } else {
            ImGui::Text("Search results:");
            auto objects = scene->findObjectsFuzzy(find_str);
            for(auto o : objects) {
                std::string type_name = o->get_derived_info().m_type.get_name().to_string();
                std::string name_with_uid;
                if(rttr::type::get<GameObject>() != o->get_type()) {
                    name_with_uid = MKSTR(o->getName() << " [" << type_name << "]" << "##" << o);
                } else {
                    name_with_uid = MKSTR(o->getName() << "##" << o);
                }

                if(ImGui::Selectable(name_with_uid.c_str(), o == editor->getSelectedObject())) {
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
            }
        }

        ImGui::Separator();
        ImGui::Text("Scene controllers:");
        ImGui::Separator();
        for(size_t i = 0; i < scene->controllerCount(); ++i) {
            SceneController* c = scene->getController(i);
            if(ImGui::CollapsingHeader(c->get_type().get_name().to_string().c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                c->onGui();
            }            
        }

        ImGui::End();
    }
}

void EditorSceneInspector::sceneTreeViewNode(GameObject* o, Editor* editor) {
    std::string type_name = o->get_derived_info().m_type.get_name().to_string();
    std::string name;
    std::string name_with_uid;
    if(rttr::type::get<GameObject>() != o->get_type()) {
        name = MKSTR(o->getName() << " [" << type_name << "]");
        name_with_uid = MKSTR(o->getName() << " [" << type_name << "]" << "##" << o);
    } else {
        name = o->getName();
        name_with_uid = MKSTR(o->getName() << "##" << o);
    }

    ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if(o->childCount() == 0) {
        ImGui::PushID(name_with_uid.c_str());
        ImGui::TreeAdvanceToLabelPos();
        if(ImGui::Selectable(name_with_uid.c_str(), o == editor->getSelectedObject())) {
            editor->setSelectedObject(o);
        }
        if(ImGui::BeginDragDropSource(0)) {
            ImGui::SetDragDropPayload("DND_OBJECT", &o, sizeof(o));
            ImGui::Text(name_with_uid.c_str());
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
            ImGuiTreeNodeFlags_OpenOnDoubleClick |
            ImGuiTreeNodeFlags_OpenOnArrow, 
            ""
        );
        ImGui::SameLine();
        if(ImGui::Selectable(name_with_uid.c_str(), o == editor->getSelectedObject())) {
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