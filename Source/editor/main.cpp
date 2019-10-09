#include "../common/engine.hpp"

#include "editor.hpp"
#include "game_state.hpp"

#include "../common/resource/resource_desc_library.hpp"
#include "../common/input_listener.hpp"

#include "../katana/timer.hpp"
#include "../common/katana_impl.hpp"

#include "thumb_builder.hpp"

std::unique_ptr<KatanaImpl> kt_play_mode;

class EscInputListener : public InputListenerWrap {
public:
    EscInputListener() {
        bindActionPress("EndPlayMode", [](){
            kt_play_mode.reset(0);
        });
    }
};

#include "../common/ecs/ecs_world.hpp"


class archTest : public ktEcsArchetype<double, int> {
public:
    double getDouble() { return std::get<0>(attribs); }
    int getInt() { return std::get<1>(attribs); }
    void setDouble(double d) { std::get<0>(attribs) = d; }
};
class archTest2 : public ktEcsArchetype<int, std::string> {
public:
    
};

class ecsTestSys : public ktEcsSystem<archTest, archTest2> {
    std::set<archTest*> tests;
    std::set<archTest2*> tests2;
public:
    void onFit(archTest* test) {
        tests.insert(test);
        LOG("archTest fit");
    }
    void onUnfit(archTest* ptr) {
        tests.erase(ptr);
        LOG("archTest unfit");
    }
    void onFit(archTest2* test) {
        tests2.insert(test);
        LOG("archTest2 fit");
    }
    void onUnfit(archTest2* ptr) {
        tests2.erase(ptr);
        LOG("archTest2 unfit");
    }
    void onInvoke() {
        for(auto& t : tests) {
            LOG("ecsTestSys: " << t->getDouble() << ", " << t->getInt());
            double d = t->getDouble();
            t->setDouble(d += 0.01);
        }
        for(auto& t : tests2) {
            LOG("ecsTest2");
        }
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

    ktEcsWorld world;
    world.addSystem<ecsTestSys>();
    ktEntity ent = world.createEntity();
    ktEntity ent2 = world.createEntity();
    world.createAttrib<int>(ent);
    world.createAttrib<double>(ent);
    world.createAttrib<std::string>(ent);
    world.createAttrib<int>(ent2);
    world.createAttrib<std::string>(ent2);
    world.removeAttrib<std::string>(ent);

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

            world.update();

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