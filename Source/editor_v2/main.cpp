#include "../common/platform/platform.hpp"
#include "../common/util/log.hpp"

#include "editor.hpp"

int main() {
    if(!platformInit()) {
        LOG_ERR("Failed to initialize platform");
        return 0;
    }

    // TODO : 
    input().getTable().addAxisKey("MoveHori", "KB_D", 1.0f);
    input().getTable().addAxisKey("MoveHori", "KB_A", -1.0f);
    input().getTable().addAxisKey("MoveVert", "KB_W", 1.0f);
    input().getTable().addAxisKey("MoveVert", "KB_S", -1.0f);
    input().getTable().addActionKey("Attack", "KB_SPACE");
    input().getTable().addActionKey("SlowWalk", "KB_LEFT_ALT");
    //

    editorState().game_state.reset(new GameState());

    Editor editor;
    editor.init();
    while(!platformIsShuttingDown()) {
        platformUpdate();
        unsigned w, h;
        unsigned cx, cy;
        platformGetViewportSize(w, h);
        platformGetMousePos(cx, cy);
        if(editorState().is_play_mode) {
            editorState().game_state->update(w, h);
        } else {
            editor.update(w, h, cx, cy);
        }
        platformSwapBuffers();
    }
    editor.cleanup();

    platformCleanup();
    return 0;
}