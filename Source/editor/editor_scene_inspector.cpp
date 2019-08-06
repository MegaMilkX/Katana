#include "editor_scene_inspector.hpp"

#include "../common/lib/imgui_wrap.hpp"
#include "../common/lib/imgui/imgui_internal.h"
#include "../common/util/log.hpp"

#include "editor.hpp"

//#include "../common/scene/third_person_camera.hpp"

#include "../common/lib/nativefiledialog/nfd.h"

#include "../common/util/has_suffix.hpp"

static bool showSaveGameObjectDialog(ktNode* o, bool forceDialog) {
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

static ktNode* guiGameObjectContextMenu(ktNode* o, ObjectSet& selected) {
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
    //update(editor->getScene(), editor->getSelectedObjects(), title);
}

void EditorSceneInspector::update(GameScene* scene, ObjectSet& selected, const std::string& title) {
    if(ImGui::Begin(title.c_str(), 0, ImGuiWindowFlags_MenuBar)) {
        if(ImGui::BeginMenuBar()) {
            if(ImGui::BeginMenu("Create node...")) {
                if(ImGui::MenuItem("Empty")) {
                    selected.clearAndAdd(scene->createChild());    
                }
                auto& table = getAttribTypeLib().getTable();
                for(auto& kv : table) {
                    if(ImGui::BeginMenu(kv.first.c_str())) {
                        for(auto t : kv.second) {
                            if(ImGui::MenuItem(t.get_name().to_string().c_str())) {
                                auto o = scene->createChild();
                                o->get(t);
                                selected.clearAndAdd(o);
                            }
                        }
                        ImGui::EndMenu();
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        if(ImGui::InputText("Find", search_string_buf, 256)) {

        }
        std::string find_str = search_string_buf;
        if(find_str.empty()) {
            sceneTreeViewNode(scene, selected);
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
                        ktNode* tgt_dnd_so = *(ktNode**)payload->Data;
                        o->takeOwnership(tgt_dnd_so);
                    }
                    ImGui::EndDragDropTarget();
                }
                o = guiGameObjectContextMenu(o, selected);
            }
        }
/*
        ImGui::Separator();
        ImGui::Text("Scene controllers:");
        ImGui::Separator();
        for(size_t i = 0; i < scene->controllerCount(); ++i) {
            SceneController* c = scene->getController(i);
            if(ImGui::CollapsingHeader(c->get_type().get_name().to_string().c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                c->onGui();
            }            
        }
*/
        ImGui::End();
    }
}

void EditorSceneInspector::sceneTreeViewNode(ktNode* o, ObjectSet& selected) {
    std::string name_with_uid = MKSTR(o->getName() << "###" << o);

    ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    std::string icons;
    if(!o->isEnabled()) {
        icons += ICON_MDI_EYE_OFF;
    }
    for(size_t i = 0; i < o->componentCount(); ++i) {
        auto& c = o->getById(i);
        icons += c->getIconCode();
    }
    if(!icons.empty()) {
        icons += " ";
    }

    ImVec4 label_col = ImVec4(1,1,1,1);
    if(o->getType() == OBJECT_INSTANCE) {
        label_col = ImVec4(0.4f, 1.0f, 0.7f, 1.0f);
    } else if(o->getFlags() && OBJECT_FLAG_TRANSIENT) {
        label_col = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    }

    if(o->childCount() == 0 || o->getType() == OBJECT_INSTANCE) {
        ImGui::PushID(name_with_uid.c_str());
        ImGui::TreeAdvanceToLabelPos();
        ImGui::PushStyleColor(ImGuiCol_Text, label_col);
        if(ImGui::Selectable(MKSTR(icons << name_with_uid).c_str(), selected.contains(o))) {
            selected.clearAndAdd(o);
        }
        ImGui::PopStyleColor();
        if(ImGui::BeginDragDropSource(0)) {
            ImGui::SetDragDropPayload("DND_OBJECT", &o, sizeof(o));
            ImGui::Text(name_with_uid.c_str());
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_OBJECT")) {
                ktNode* tgt_dnd_so = *(ktNode**)payload->Data;
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
        ImGui::PushStyleColor(ImGuiCol_Text, label_col);
        if(ImGui::Selectable(MKSTR(icons << name_with_uid).c_str(), selected.contains(o))) {
            selected.clearAndAdd(o);
        }
        ImGui::PopStyleColor();
        if(ImGui::BeginDragDropSource(0)) {
            ImGui::SetDragDropPayload("DND_OBJECT", &o, sizeof(o));
            ImGui::Text(o->getName().c_str());
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_OBJECT")) {
                ktNode* tgt_dnd_so = *(ktNode**)payload->Data;
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