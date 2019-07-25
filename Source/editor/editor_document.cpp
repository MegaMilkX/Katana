#include "editor_document.hpp"

#include "editor.hpp"

#include "../common/resource/resource_tree.hpp"

EditorDocument::EditorDocument(ResourceNode* node)
: _node(node) {
    if(node) {
        _name = MKSTR(node->getFullName() << "##" << this);
    } else {
        _name = MKSTR("* Untitled" << "##" << this);
    }
}


const std::string& EditorDocument::getName() const {
    return _name;
}


void EditorDocument::update (Editor* ed) {
    if(ImGui::Begin(getName().c_str(), &is_open)) {
        if(ImGui::IsRootWindowOrAnyChildFocused()) {
            ed->getResourceTree().setSelected(_node);
            ed->setCurrentDockspace(ImGui::GetWindowDockID());
        }
        
        onGui(ed);

        ImGui::End();
    }
}