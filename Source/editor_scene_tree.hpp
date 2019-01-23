#ifndef EDITOR_SCENE_TREE_HPP
#define EDITOR_SCENE_TREE_HPP

#include "editor_window.hpp"
#include "editor_object_inspector.hpp"
#include "editor_scene.hpp"
#include "editor_state.hpp"
#include "scene.hpp"

class EditorSceneTree : public EditorWindow {
public:
    EditorSceneTree(EditorObjectInspector* object_inspector, EditorScene* scene_window)
    : object_inspector(object_inspector),
    scene_window(scene_window) { }
    std::string Name() {
        return "Scene tree";
    }
    void Update() {
        if(!scene) return;
        if(ImGui::Button("Create Object")) {
            SceneObject* so = scene->createObject();
            so->setName("new_object");
            editorState().selected_object = so;
        }
        ImGui::Separator();
        sceneTreeViewNode(scene->getRootObject());

        ImGui::Separator();

        //scene->_editorGuiComponentList();
        //ImGui::Separator();
        ImGui::Text("ResourceList");
        scene->_editorGuiResourceList();

        /*
        for(unsigned i = 0; i < scene->getRootObject()->childCount(); ++i) {
            sceneTreeViewNode(scene->getRootObject()->getChild(i));
        }
        */
    }

    void setScene(Scene* scene) {
        this->scene = scene;
    }

private:
    void sceneTreeViewNode(SceneObject* so) {
        if(so->childCount() == 0) {
            ImGui::PushID(MKSTR(so->getName() << "##" << so).c_str());
            if(ImGui::Selectable(MKSTR(so->getName() << "##" << so).c_str(), editorState().selected_object == so ? true : false)) {
                editorState().selected_object = (so);
            }
            SceneObject* dnd_so = so;
            if(ImGui::BeginDragDropSource(0)) {
                ImGui::SetDragDropPayload("DND_OBJECT", &dnd_so, sizeof(dnd_so));
                ImGui::Text(MKSTR("Object " << so->getName()).c_str());
                ImGui::EndDragDropSource();
            }
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_OBJECT")) {
                    SceneObject* tgt_dnd_so = *(SceneObject**)payload->Data;
                    tgt_dnd_so->setParent(so);
                }
                ImGui::EndDragDropTarget();
            }
            ImGui::PopID();
        } else {
            ImGui::PushID(so);
            bool node_open = ImGui::TreeNodeEx(
                (void*)so, 
                (editorState().selected_object == so ? ImGuiTreeNodeFlags_Selected : 0) | 
                ImGuiTreeNodeFlags_OpenOnDoubleClick |
                ImGuiTreeNodeFlags_OpenOnArrow, 
                MKSTR(so->getName()).c_str()
            );
            if(ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                editorState().selected_object = (so);
            }
            
            SceneObject* dnd_so = so;
            if(ImGui::BeginDragDropSource(0)) {
                ImGui::SetDragDropPayload("DND_OBJECT", &dnd_so, sizeof(dnd_so));
                ImGui::Text(MKSTR("Object " << so->getName()).c_str());
                ImGui::EndDragDropSource();
            } else {
                
            }
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_OBJECT")) {
                    SceneObject* tgt_dnd_so = *(SceneObject**)payload->Data;
                    tgt_dnd_so->setParent(so);
                }
                ImGui::EndDragDropTarget();
            }
            ImGui::PopID();

            if(node_open) {
                for(unsigned i = 0; i < so->childCount(); ++i) {
                    sceneTreeViewNode(so->getChild(i));
                }
                ImGui::TreePop();
            }           
        }
    }

    EditorObjectInspector* object_inspector;
    EditorScene* scene_window;
    Scene* scene = 0;
};

#endif
