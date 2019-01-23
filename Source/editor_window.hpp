#ifndef EDITOR_WINDOW_HPP
#define EDITOR_WINDOW_HPP

#include <string>

#include "lib/imgui_wrap.hpp"
#include "lib/imgui/imgui_internal.h"

class EditorWindow {
public:
    virtual ~EditorWindow() {}
    virtual std::string Name() {
        return "Window";
    }
    virtual void Update() = 0;

    bool is_open = true;
};



#endif
