#ifndef EDITOR_STATE_HPP
#define EDITOR_STATE_HPP

#include "scene.hpp"

struct EditorState {
    SceneObject* selected_object = 0;
};

inline EditorState& editorState() {
    static EditorState state;
    return state;
}

#endif
