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

        if(nested_window_stack.empty()) {
            onGui(ed, dt);
        } else {
           if(ImGui::SmallButton("Root")) {
               nested_window_stack.clear();
           }
           for(size_t i = 0; i < nested_window_stack.size(); ++i) {
               ImGui::SameLine(); ImGui::Text(">"); ImGui::SameLine();
               if(ImGui::SmallButton(MKSTR(nested_window_stack[i]->getIconCode() << " " << nested_window_stack[i]->getTitle()).c_str())) {
                   nested_window_stack.resize(i + 1);
               }
           }

           if(nested_window_stack.empty()) {
               onGui(ed, dt);
           } else {
               nested_window_stack.back()->onGui(ed, dt);
           }
        }
    }
    ImGui::End();
}
