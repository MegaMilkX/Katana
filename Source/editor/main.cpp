#include "../common/platform/platform.hpp"
#include "../common/util/log.hpp"

#include "editor.hpp"
#include "game_state.hpp"

#include "../common/components/volume_trigger_test.hpp"

#include "../common/resource/resource_desc_library.hpp"

#include "../katana/timer.hpp"
#include "../common/katana_impl.hpp"

std::unique_ptr<KatanaImpl> kt_play_mode;

class EscInputListener : public InputListenerWrap {
public:
    EscInputListener() {
        bindActionPress("EndPlayMode", [](){
            kt_play_mode.reset(0);
        });
    }
};

int main(int argc, char* argv[]) {
    if(!platformInit()) {
        LOG_ERR("Failed to initialize platform");
        return 0;
    }

    ResourceDescLibrary::get()->init();

    // TODO : 
    input().getTable().addAxisKey("MoveCamX", "MOUSE_X", 1.0);
    input().getTable().addAxisKey("MoveCamY", "MOUSE_Y", 1.0);
    input().getTable().addAxisKey("CameraZoom", "MOUSE_SCROLL", 1.0);
    input().getTable().addAxisKey("MoveHori", "KB_D", 1.0f);
    input().getTable().addAxisKey("MoveHori", "KB_A", -1.0f);
    input().getTable().addAxisKey("MoveVert", "KB_W", 1.0f);
    input().getTable().addAxisKey("MoveVert", "KB_S", -1.0f);
    input().getTable().addActionKey("Attack", "KB_SPACE");
    input().getTable().addActionKey("SlowWalk", "KB_LEFT_ALT");
    input().getTable().addActionKey("EndPlayMode", "KB_ESCAPE");
    //

    EscInputListener esc_input_listener;

    std::unique_ptr<AppState> app_state;

    for(int i = 0; i < argc; ++i) {
        if(std::string(argv[i]) == "play") {
            std::string scene_path;
            if(argc > i + 1) {
                scene_path = argv[i + 1];
            }
            app_state.reset(new GameState(scene_path));
        }
    }
    if(!app_state) {
        app_state.reset(new Editor());
    }

    timer frameTimer;

    //try {
        app_state->onInit();
        while(!platformIsShuttingDown()) {
            frameTimer.start();

            platformUpdate();
            unsigned w, h;
            unsigned cx, cy;
            platformGetViewportSize(w, h);
            platformGetMousePos(cx, cy);

            if(kt_play_mode) {
                kt_play_mode->update();
            } else if(app_state) {
                app_state->onUpdate();
                app_state->onGui();
                app_state->onRender(w, h);
            }
            
            platformSwapBuffers();
            if(kt_play_mode) {
                kt_play_mode->update_time(frameTimer.end());
            }
        }
        app_state->onCleanup();
    //} catch(std::exception& ex) {
    //    LOG_ERR(ex.what());
    //}

    platformCleanup();
    return 0;
}