#include "imgui_helpers.hpp"

#include "../editor/editor.hpp"

void tryOpenDocument(const std::string& res_path) {
#ifdef KT_EDITOR
    if(!Editor::get()) {
        LOG_WARN("Editor is null");
        return;
    }
    Editor::get()->tryOpenDocument(res_path);
#endif
}

