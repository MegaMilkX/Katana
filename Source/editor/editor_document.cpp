#include "editor_document.hpp"

#include "editor.hpp"

#include "../common/resource/resource_tree.hpp"

#include "../common/util/filesystem.hpp"
#include "../common/platform/platform.hpp"

#include "../common/resource/ext_to_icon.hpp"

EditorDocument::EditorDocument()
{
    is_unsaved = true;
    _window_name = MKSTR("Untitled" << "###" << this);
}

ResourceNode* EditorDocument::getNode() {
    return _node.lock().get();
}

const std::string& EditorDocument::getName() const {
    return _name;
}
const std::string& EditorDocument::getWindowName() const {
    return _window_name;
}

void EditorDocument::saveAs() {
    
}

bool EditorDocument::saveResource(std::shared_ptr<Resource>& r, const std::string& path) {
    if(r->write_to_file(path)) {
        gResourceTree.scanFilesystem(get_module_dir() + "/" + platformGetConfig().data_dir);
        return true;
    }
    return false;
}

void EditorDocument::undo() {

}
void EditorDocument::redo() {

}
void EditorDocument::backup() {

}

void EditorDocument::update (Editor* ed) {
    ImGuiWindowFlags flags = imgui_win_flags;
    if(isUnsaved()) {
        flags |= ImGuiWindowFlags_UnsavedDocument;
    }
    std::string win_title = getWindowName();
    if(_node.expired() && !_name.empty()) {
        win_title = "(deleted) " + getWindowName();
    }
    std::string ext;
    if(_name.find_last_of(".") != _name.npos) {
         ext = _name.substr(_name.find_last_of("."));
    }
    std::string icon = getExtIconCode(ext.c_str());
    if(ImGui::Begin(MKSTR(icon << " " << win_title).c_str(), &is_open, flags)) {
        if(ImGui::IsRootWindowOrAnyChildFocused()) {
            ed->getResourceTree().setSelected(_node.lock().get());
            ed->setCurrentDockspace(ImGui::GetWindowDockID());
            ed->setFocusedDocument(this);
        }
        
        onGui(ed);
    }
    ImGui::End();
}