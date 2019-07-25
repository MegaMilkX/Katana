#include "editor_scene_inspector.hpp"

#include "../common/lib/imgui_wrap.hpp"
#include "../common/lib/imgui/imgui_internal.h"
#include "../common/util/log.hpp"

#include "editor.hpp"

#include "../common/scene/third_person_camera.hpp"

#include "../common/lib/nativefiledialog/nfd.h"

static bool showSaveGameObjectDialog(GameObject* o, bool forceDialog) {
    char* outPath;
    auto r = NFD_SaveDialog("so", NULL, &outPath);
    if(r == NFD_OKAY) {
        std::string filePath(outPath);
        if(!has_suffix(filePath, ".so")) {
            filePath = filePath + ".so";
        }
        std::cout << filePath << std::endl;
        if(o->write(filePath)) {
            LOG("Game object saved");
        } else {
            LOG_WARN("Failed to save game object");
        }
    }
    
    return true;
}

static GameObject* guiGameObjectContextMenu(GameObject* o, ObjectSet& selected) {
    if (ImGui::BeginPopupContextItem()) {
        if(ImGui::MenuItem("Create child")) {
            selected.clearAndAdd(o->createChild());
            // TODO: editor->backupScene("child created");
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
            // TODO: editor->backupScene("object duplicated");
        }
        ImGui::Separator();
        if(ImGui::MenuItem("Save As...")) {
            showSaveGameObjectDialog(o, true);
        }
        ImGui::Separator();
        if(ImGui::MenuItem("Delete")) {
            if(selected.contains(o)) selected.clear();
            o->remove(true);
            o = 0;
            // TODO: editor->backupScene("object deleted");
        }
        if(ImGui::MenuItem("Delete tree")) {
            if(selected.contains(o)) selected.clear();
            o->remove();
            o = 0;
            // TODO: editor->backupScene("object tree deleted");
        }
        ImGui::EndPopup();
    }
    return o;
} 

void EditorSceneInspector::update(Editor* editor, const std::string& title) {
    update(editor->getScene(), editor->getSelectedObjects(), title);
}

void EditorSceneInspector::update(GameScene* scene, ObjectSet& selected, const std::string& title) {
    if(ImGui::Begin(title.c_str(), 0, ImGuiWindowFlags_MenuBar)) {
        if(ImGui::BeginMenuBar()) {
            if(ImGui::MenuItem("Create")) {
                selected.clearAndAdd(scene->getRoot()->createChild());
                // TODO: editor->backupScene("object created");
            }
            ImGui::EndMenuBar();
        }

        if(ImGui::InputText("Find", search_string_buf, 256)) {

        }
        std::string find_str = search_string_buf;
        if(find_str.empty()) {
            sceneTreeViewNode(scene->getRoot(), selected);
        } else {
            ImGui::Text("Search results:");
            auto objects = scene->findObjectsFuzzy(find_str);
            for(auto o : objects) {
                std::string name_with_uid;
                name_with_uid = MKSTR(o->getName() << "##" << o);

                if(ImGui::Selectable(name_with_uid.c_str(), selected.contains(o))) {
                    selected.clearAndAdd(o);
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
                o = guiGameObjectContextMenu(o, selected);
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

void EditorSceneInspector::sceneTreeViewNode(GameObject* o, ObjectSet& selected) {
    std::string name_with_uid = MKSTR(o->getName() << "##" << o);

    ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if(o->childCount() == 0) {
        ImGui::PushID(name_with_uid.c_str());
        ImGui::TreeAdvanceToLabelPos();
        if(ImGui::Selectable(name_with_uid.c_str(), selected.contains(o))) {
            selected.clearAndAdd(o);
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
        guiGameObjectContextMenu(o, selected);
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
        if(ImGui::Selectable(name_with_uid.c_str(), selected.contains(o))) {
            selected.clearAndAdd(o);
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
        o = guiGameObjectContextMenu(o, selected);
        ImGui::PopID();

        if(node_open) {
            if(o) {
                for(unsigned i = 0; i < o->childCount(); ++i) {
                    sceneTreeViewNode(o->getChild(i), selected);
                }
            }
            ImGui::TreePop();
        }
    }
}