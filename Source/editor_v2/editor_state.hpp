#ifndef EDITOR_STATE_HPP
#define EDITOR_STATE_HPP

#include "game_state.hpp"

struct EditorState {
    bool is_play_mode = false;
    std::shared_ptr<GameState> game_state;
};

inline EditorState& editorState() {
    static EditorState s;
    return s;
}

#endif
