#include "editor_window.hpp"

#include "editor.hpp"

void EditorWindow::draw (Editor* ed, float dt) {
    ImGuiWindowFlags flags = imgui_win_flags;
    if(isUnsaved()) {
        flags |= ImGuiWindowFlags_UnsavedDocument;
    }
    std::string win_title = getTitle();
    //if(_node.expired() && !_name.empty()) {
    //    win_title = "(deleted) " + getTitle();
    //}
    //std::string ext;
    //if(_name.find_last_of(".") != _name.npos) {
    //     ext = _name.substr(_name.find_last_of("."));
    //}
    std::string icon = getIconCode();// getExtIconCode(ext.c_str());
    if(ImGui::Begin(MKSTR(icon << " " << win_title).c_str(), &_is_open, flags)) {
        if(ImGui::IsRootWindowOrAnyChildFocused()) {
            //ed->getResourceTree().setSelected(_node.lock().get());
            ed->setCurrentDockspace(ImGui::GetWindowDockID());
            ed->setFocusedWindow(this);
        }
        
        onGui(ed, dt);
    }
    ImGui::End();
}
