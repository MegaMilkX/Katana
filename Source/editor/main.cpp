#include "../common/engine.hpp"

#include "editor.hpp"
#include "game_state.hpp"

#include "../common/resource/resource_desc_library.hpp"
#include "../common/input_listener.hpp"

#include "../katana/timer.hpp"
#include "../common/katana_impl.hpp"

#include "thumb_builder.hpp"

#include "../common/input/draft/input_mgr.hpp"
#include "../common/input/draft/input_device_xinput_pad.hpp"

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
    if(!katanaInit()) {
        LOG_ERR("Failed to initialize engine");
        return 0;
    }

    ResourceDescLibrary::get()->init();

    if(!ThumbBuilder::get()->init()) {
        LOG_ERR("Failed to init thumb builder");
        return 0;
    }

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

    getInputMgr().setUserCount(1);
    InputDevice* device = new InputDeviceXInputPad(0);
    getInputMgr().addDevice(device);
    getInputMgr().addDevice(new InputDeviceXInputPad(1));
    getInputMgr().addDevice(new InputDeviceXInputPad(2));
    getInputMgr().addDevice(new InputDeviceXInputPad(3));

    InputAction action;
    InputAction actionJump;
    InputAction actionBack;
    ButtonCombo comb(rttr::type::get<InputAdapterXboxPad>(), 0, 10);
    action.inputs.push_back(comb);
    actionJump.inputs.push_back(ButtonCombo(rttr::type::get<InputAdapterXboxPad>(), 10));
    actionBack.inputs.push_back(ButtonCombo(rttr::type::get<InputAdapterXboxPad>(), 11));
    getInputMgr().addAction("Test action", action);
    getInputMgr().addAction("Jump", actionJump);
    getInputMgr().addAction("Back", actionBack);
    

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
    float dt = 1.0f/60.0f;
    //try {
        app_state->onInit();
        while(!platformIsShuttingDown()) {
            frameTimer.start();

            getInputMgr().update();
            platformUpdate(dt);
            unsigned w, h;
            unsigned cx, cy;
            platformGetViewportSize(w, h);
            platformGetMousePos(cx, cy);

            if(kt_play_mode) {
                kt_play_mode->update();
            } else if(app_state) {
                app_state->onUpdate();
                app_state->onGui(dt);
                app_state->onRender(w, h);
            }
            
            platformSwapBuffers();
            dt = frameTimer.end();
            if(kt_play_mode) {
                kt_play_mode->update_time(dt);
            }

            ThumbBuilder::get()->poll();
        }
        app_state->onCleanup();
    //} catch(std::exception& ex) {
    //    LOG_ERR(ex.what());
    //}

    ThumbBuilder::get()->cleanup();

    katanaCleanup();
    return 0;
}