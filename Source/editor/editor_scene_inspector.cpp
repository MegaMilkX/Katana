#include "editor_scene_inspector.hpp"

#include "../common/lib/imgui_wrap.hpp"
#include "../common/lib/imgui/imgui_internal.h"
#include "../common/util/log.hpp"

#include "editor.hpp"

#include "scene/third_person_camera.hpp"

static GameObject* guiGameObjectContextMenu(GameObject* o, Editor* editor) {
    if (ImGui::BeginPopupContextItem()) {
        if(ImGui::MenuItem("Create child")) {
            editor->setSelectedObject(
                o->createChild()
            );
            editor->backupScene("child created");
        }
        if(o && (o->getParent() != 0)) {
            ImGui::Separator();
            if(ImGui::MenuItem("Move to top")) {
                // TODO:
            }
        }
        ImGui::Separator();
        if(ImGui::MenuItem("Duplicate")) {
            o->duplicate();
            editor->backupScene("object duplicated");
        }
        ImGui::Separator();
        if(ImGui::MenuItem("Delete")) {
            if(editor->getSelectedObject() == o)editor->setSelectedObject(0);
            o->remove(true);
            o = 0;
            editor->backupScene("object deleted");
        }
        if(ImGui::MenuItem("Delete tree")) {
            if(editor->getSelectedObject() == o) editor->setSelectedObject(0);
            o->remove();
            o = 0;
            editor->backupScene("object tree deleted");
        }
        ImGui::EndPopup();
    }
    return o;
} 

void EditorSceneInspector::update(Editor* editor) {
    GameScene* scene = editor->getScene();

    if(ImGui::Begin("Scene Inspector", 0, ImGuiWindowFlags_MenuBar)) {
        if(ImGui::BeginMenuBar()) {
            if(ImGui::MenuItem("Create")) {
                editor->setSelectedObject(editor->getScene()->getRoot()->createChild());
                editor->backupScene("object created");
            }
            ImGui::EndMenuBar();
        }

        if(ImGui::InputText("Find", search_string_buf, 256)) {

        }
        std::string find_str = search_string_buf;
        if(find_str.empty()) {
            sceneTreeViewNode(scene->getRoot(), editor);
        } else {
            ImGui::Text("Search results:");
            auto objects = scene->findObjectsFuzzy(find_str);
            for(auto o : objects) {
                std::string name_with_uid;
                name_with_uid = MKSTR(o->getName() << "##" << o);

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
                        o->takeOwnership(tgt_dnd_so);
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
    std::string name_with_uid = MKSTR(o->getName() << "##" << o);

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
                o->takeOwnership(tgt_dnd_so);
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
                o->takeOwnership(tgt_dnd_so);
            }
            ImGui::EndDragDropTarget();
        }
        o = guiGameObjectContextMenu(o, editor);
        ImGui::PopID();

        if(node_open) {
            if(o) {
                for(unsigned i = 0; i < o->childCount(); ++i) {
                    sceneTreeViewNode(o->getChild(i), editor);
                }
            }
            ImGui::TreePop();
        }
    }
}