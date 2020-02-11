#include "editor_window.hpp"

#include "editor.hpp"

void EditorWindow::drawInternal(Editor* ed, float dt) {
    if(!nested_window) {
        onGui(ed, dt);
    } else {
        if(ImGui::SmallButton("Root")) {
            setNestedWindow(0);
        }
        EditorWindow* cur_nested = nested_window.get();
        while (cur_nested) {
            ImGui::SameLine(); ImGui::Text(">"); ImGui::SameLine();
            if (ImGui::SmallButton(MKSTR(cur_nested->getIconCode() << " " << cur_nested->getTitle()).c_str())) {
                cur_nested->setNestedWindow(0);
            }
            if(!cur_nested->getNestedWindow()) {
                break;
            } else {
                cur_nested = cur_nested->getNestedWindow();
            }
        }

        if(!cur_nested) {
            onGui(ed, dt);
        } else {
            cur_nested->onGui(ed, dt);
        }
    }
}

void EditorWindow::drawAsRoot(Editor* ed, float dt) {
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

        drawInternal(ed, dt);
    }
    ImGui::End();
}

void EditorWindow::drawToolbox(Editor* ed) {
    if(!nested_window) {
        onGuiToolbox(ed);
    } else {
        if(ImGui::SmallButton("Root")) {
            setNestedWindow(0);
        }
        EditorWindow* cur_nested = nested_window.get();
        while (cur_nested) {
            ImGui::SameLine(); ImGui::Text(">"); ImGui::SameLine();
            if (ImGui::SmallButton(MKSTR(cur_nested->getIconCode() << " " << cur_nested->getTitle()).c_str())) {
                cur_nested->setNestedWindow(0);
            }
            if(!cur_nested->getNestedWindow()) {
                break;
            } else {
                cur_nested = cur_nested->getNestedWindow();
            }
        }

        if(!cur_nested) {
            onGuiToolbox(ed);
        } else {
            cur_nested->onGuiToolbox(ed);
        }
    }
}