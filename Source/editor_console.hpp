#ifndef EDITOR_CONSOLE_HPP
#define EDITOR_CONSOLE_HPP

#include "editor_window.hpp"

class EditorConsole : public EditorWindow {
public:
    std::string Name() {
        return "Console";
    }
    void Update() {
    }
};

#endif
