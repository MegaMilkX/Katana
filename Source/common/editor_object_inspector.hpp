#ifndef EDITOR_OBJECT_INSPECTOR_HPP
#define EDITOR_OBJECT_INSPECTOR_HPP

#include "editor_window.hpp"
#include "scene.hpp"

class EditorObjectInspector : public EditorWindow {
public:
    std::string Name() {
        return "Object inspector";
    }
    void Update() {
        if(!editorState().selected_object) {
            ImGui::Text("Nothing selected");
            return;
        }

        std::string buf = editorState().selected_object->getName();
        buf.resize(256);
        if(ImGui::InputText("Name", (char*)buf.data(), buf.size())) {
            editorState().selected_object->setName(buf);
        }
        if(ImGui::Button("Delete object")) {
            editorState().selected_object->getScene()->removeObject(
                editorState().selected_object
            );
            editorState().selected_object = 0;
        } else {
            editorState().selected_object->_editorGui();
        }
    }
};

#endif
