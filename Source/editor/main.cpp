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
#include "../common/input/hid/hid.hpp"
#include "../common/audio.hpp"

#include "../common/util/threading/delegated_call.hpp"

std::unique_ptr<KatanaImpl> kt_play_mode;

class EscInputListener : public InputListenerWrap {
public:
    EscInputListener() {
        bindActionPress("EndPlayMode", [](){
            kt_play_mode.reset(0);
        });
    }
};

#include "../common/render/vertex_format.hpp"

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

    VERTEX_FMT::GENERIC::vertexSize();
    for(int i = 0; i < VERTEX_FMT::GENERIC::attribCount(); ++i) {
        auto& dsc = VERTEX_FMT::GENERIC::getAttribDesc(i);
        LOG_WARN(dsc.name << ": " << dsc.count << ", " << dsc.gl_type << ", " << dsc.normalized);
    }
    
    std::vector<const char*> array(VERTEX_FMT::GENERIC::attribCount());
    VERTEX_FMT::GENERIC::makeOutAttribNameArray(array.data());
    for(int i = 0; i < VERTEX_FMT::GENERIC::attribCount(); ++i) {
        LOG_WARN(array[i]);
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


    auto clip0 = retrieve<AudioClip>("audio/sfx/mgs.ogg");
    auto clip1 = retrieve<AudioClip>("audio/sfx/hit.ogg");
    auto clip2 = retrieve<AudioClip>("audio/sfx/gravel1.ogg");
    auto clip3 = retrieve<AudioClip>("audio/sfx/swoosh.ogg");

    getInputMgr().setUserCount(1);
    InputDevice* device = new InputDeviceXInputPad(0);
    getInputMgr().addDevice(device);
    getInputMgr().addDevice(new InputDeviceXInputPad(1));
    getInputMgr().addDevice(new InputDeviceXInputPad(2));
    getInputMgr().addDevice(new InputDeviceXInputPad(3));

    InputAction action;
    InputAction actionJump;
    InputAction actionBack;
    InputAxis   axis;
    ButtonCombo comb(rttr::type::get<InputAdapterXboxPad>(), 0);
    action.inputs.push_back(comb);
    actionJump.inputs.push_back(ButtonCombo(rttr::type::get<InputAdapterXboxPad>(), 10));
    actionBack.inputs.push_back(ButtonCombo(rttr::type::get<InputAdapterXboxPad>(), 11));
    getInputMgr().setAction("Test action", action);
    getInputMgr().setAction("Jump", actionJump);
    getInputMgr().setAction("Back", actionBack);
    axis.keys.push_back(InputAxisKey{rttr::type::get<InputAdapterXboxPad>(), KEY_XBOX_LEFT_STICK_X, 1.0f});
    getInputMgr().setAxis("MoveX", axis);

    InputListenerHdl menu_hdl;
    InputListenerHdl hdl;
    menu_hdl.bindPress("Jump", [&clip0](){
        audio().playOnce(clip0->getBuffer(), 0.5f);
        LOG("Pressed a menu button");
    });
    menu_hdl.bindPress("Test action", [&menu_hdl](){
        menu_hdl.moveToTop();
        LOG("Opened the menu");
    });
    menu_hdl.bindRelease("Test action", [&hdl](){
        hdl.moveToTop();
        LOG("Closed menu")
    });
    
    hdl.bindPress("Jump", [&clip1](){
        audio().playOnce(clip1->getBuffer(), 0.5f);
        LOG("Jumped");
    });
    hdl.bindPressRepeater("Jump", [&clip2](){
        audio().playOnce(clip2->getBuffer(), 1.0f);
    });
    hdl.bindTap("Jump", [&clip3](){
        audio().playOnce(clip3->getBuffer(), .5f);
        LOG("Jump tapped");
    });
    hdl.bindAxis("MoveX", [](float v){
        LOG("X: " << v);
    });
    
    
    //hidEnumDevices();
    

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

            // Delegated calls
            auto call = popDelegatedCall();
            while(call) {
                call->execute();
                call = popDelegatedCall();
            }

            //hidUpdateDevices();
            getInputMgr().update(dt);

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