#ifndef EDITOR_WINDOW_HPP
#define EDITOR_WINDOW_HPP

/*
    Editor child window specifically, not just any window
*/

#include <string>
#include <memory>
#include "../common/lib/imgui_wrap.hpp"
#include "../common/lib/imgui/imgui_internal.h"

#include "../common/util/materialdesign_icons.hpp"
#include "../common/resource/ext_to_icon.hpp"


class Editor;
class EditorWindow {
protected:
    bool _is_open = true;
    bool _is_unsaved = true;
    ImGuiWindowFlags imgui_win_flags = 0;
    std::string _window_name;
    std::string _window_title;
    std::string _icon = ICON_MDI_FILE_DOCUMENT;

    EditorWindow* nested_parent = 0;
    std::unique_ptr<EditorWindow> nested_window;

public:
    bool isOpen() const { return _is_open; }
    bool isUnsaved() const { return _is_unsaved; }
    void setOpen(bool b) { _is_open = b; }
    void markUnsaved() { _is_unsaved = true; }

    const std::string&  getName() const { return _window_name; }
    void                setName(const std::string& name) { _window_name = name; }
    const std::string&  getTitle() const { return _window_title; }
    void                setTitle(const std::string& title) { _window_title = title; }
    const std::string&  getIconCode() const { return _icon; }
    void                setIconCode(const std::string& icon_code) { _icon = icon_code; }

    void                setNestedWindow(EditorWindow* w) { if(w) w->nested_parent = this; nested_window.reset(w); }
    EditorWindow*       getNestedWindow() { return nested_window.get(); }

    virtual void save() = 0;

    virtual void onFocus() {}
    virtual void onGui(Editor* ed, float dt) = 0;
    virtual void onGuiToolbox(Editor* ed) {}

    void drawInternal(Editor* ed, float dt);
    void drawAsRoot(Editor* ed, float dt);

    void drawToolbox(Editor* ed);
};


#endif
