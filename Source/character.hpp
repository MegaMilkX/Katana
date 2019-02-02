#ifndef CHARACTER_HPP
#define CHARACTER_HPP

#include "state_machine.hpp"
#include "component.hpp"
#include "gfxm.hpp"
#include "behavior_system.hpp"
#include "input/input_mgr.hpp"

#include "transform.hpp"
#include "animator.hpp"

#include "scene_components/scene_player_info.hpp"
#include "scene.hpp"

class Character;

class CharaIdle : public State<Character> {
public:
    CharaIdle() {}
    void start(Character*) {
    }
    void update(Character*) {

    }
};

class CharaRun : public State<Character> {
public:
    void start(Character*) {
    }
    void update(Character* chara);
};

class Character : public Updatable {
    CLONEABLE_AUTO
    RTTR_ENABLE(Updatable)
    friend CharaRun;
    friend CharaIdle;
public:
    Character()
    : motion_fsm(this) {
        
    }
    ~Character() {
        ScenePlayerInfo* pi = getScene()->getSceneComponent<ScenePlayerInfo>();
        if(pi->character_transform == transform) {
            getScene()->getSceneComponent<ScenePlayerInfo>()->character_transform = 0;
        }
    }

    virtual void onCreate() {
        transform = get<Transform>();
        animator = get<Animator>();

        ScenePlayerInfo* pi = getScene()->getSceneComponent<ScenePlayerInfo>();
        pi->character_transform = transform;
    }

    virtual void init() {
        LOG("Initializing character");
        
        input_lis = input().createListener();

        input_lis->bindAxis(
            "MoveHori",
            [this](float v) {
                direction.x = v;
            }
        );
        input_lis->bindAxis(
            "MoveVert",
            [this](float v) {
                direction.y = -v;
            }
        );
        
        motion_fsm = StateMachine<Character>(this);

        state_idle.reset(new CharaIdle());
        state_run.reset(new CharaRun());
        motion_fsm.addState(state_idle.get());
        motion_fsm.addState(state_run.get());
        motion_fsm.addTransition(
            state_idle.get(),
            state_run.get(),
            [](Character* chara)->bool{
                if(gfxm::length(chara->direction) > 0.1f) {
                    return true;
                }
                return false;
            }
        );
        motion_fsm.addTransition(
            state_run.get(),
            state_idle.get(),
            [](Character* chara)->bool{
                if(gfxm::length(chara->direction) < 0.1f) {
                    return true;
                }
                return false;
            }
        );

        LOG("Done");
    }

    virtual void update() {
        motion_fsm.update();
    }

    virtual void cleanup() {
        input().removeListener(input_lis);
    }

    gfxm::vec2 direction;
private:
    Animator* animator = 0;
    Transform* transform = 0;

    InputListener* input_lis;

    StateMachine<Character> motion_fsm;
    std::shared_ptr<CharaIdle> state_idle;
    std::shared_ptr<CharaRun> state_run;
};
STATIC_RUN(Character)
{
    rttr::registration::class_<Character>("Character")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

void CharaRun::update(Character* chara) {
    Camera* current_cam = chara->getObject()->getScene()->getCurrentCamera();
    gfxm::vec4 dir = gfxm::vec4(chara->direction.x, 0.0f, chara->direction.y, 0.0f);
    dir = current_cam->get<Transform>()->getTransform() * dir;
    chara->transform->lookAt(
        chara->transform->worldPosition() + gfxm::vec3(dir.x, 0.0f, dir.z), 
        gfxm::vec3(0.0f, 1.0f, 0.0f),
        0.3f
    );
}

#endif
