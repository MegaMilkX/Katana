#ifndef EDITOR_WINDOW_HPP
#define EDITOR_WINDOW_HPP

#include <string>

#include "../common/lib/imgui_wrap.hpp"
#include "../common/lib/imgui/imgui_internal.h"

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
