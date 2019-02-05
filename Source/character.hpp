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
#include "scene_components/scene_physics_world.hpp"
#include "scene.hpp"

#include "debug_draw.hpp"

class Character;

class CharaIdle : public State<Character> {
public:
    CharaIdle() {}
    void start(Character* chara);
    void update(Character*);
};

class CharaRun : public State<Character> {
public:
    void start(Character* chara);
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
        physics_world = getScene()->getSceneComponent<PhysicsWorld>();

        ScenePlayerInfo* pi = getScene()->getSceneComponent<ScenePlayerInfo>();
        pi->character_transform = transform;
    }

    virtual void init() {
        LOG("Initializing character");

        animator->reserveLayers(3);
        
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
                if(chara->velo > 0.0001f) {
                    return true;
                }
                return false;
            }
        );
        motion_fsm.addTransition(
            state_run.get(),
            state_idle.get(),
            [](Character* chara)->bool{
                if(chara->velo < 0.0001f) {
                    return true;
                }
                return false;
            }
        );

        LOG("Done");
    }

    virtual void update() {
        direction = gfxm::normalize(direction);
        velo = gfxm::lerp(velo, gfxm::length(direction), 0.05f);
        motion_fsm.update();
        prev_pos = transform->worldPosition();
    }

    virtual void cleanup() {
        input().removeListener(input_lis);
    }

    gfxm::vec2 direction;
private:
    float velo = 0.0f;
    gfxm::vec3 prev_pos;

    Animator* animator = 0;
    Transform* transform = 0;
    PhysicsWorld* physics_world = 0;

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

void CharaIdle::start(Character* chara) {
    chara->animator->play(0, "Idle");
    chara->animator->getLayer(0).weight = 1.0f;
    chara->animator->getLayer(1).weight = 0.0f;
    chara->animator->getLayer(2).weight = 0.0f;

    chara->animator->getLayer(0).mode = SkeletonAnimLayer::BASE;
    chara->animator->getLayer(1).mode = SkeletonAnimLayer::BLEND;
    chara->animator->getLayer(2).mode = SkeletonAnimLayer::BLEND;
}
void CharaIdle::update(Character*) {

}

void CharaRun::start(Character* chara) {
    chara->animator->play(0, "Idle");
    chara->animator->play(1, "Walk");
    chara->animator->play(2, "Run");
}
void CharaRun::update(Character* chara) {
    Camera* current_cam = chara->getObject()->getScene()->getCurrentCamera();
    gfxm::vec4 dir = gfxm::vec4(chara->direction.x, 0.0f, chara->direction.y, 0.0f);
    dir = current_cam->get<Transform>()->getTransform() * dir;

    if(gfxm::length(dir) > 0.001f) {
        chara->transform->lookAt(
            chara->transform->worldPosition() + gfxm::vec3(dir.x, 0.0f, dir.z), 
            gfxm::vec3(0.0f, 1.0f, 0.0f),
            0.1f
        );
    }

    chara->animator->getLayer(0).weight = 1.0f;
    chara->animator->getLayer(1).weight = std::max(sinf(chara->velo * gfxm::pi), 0.0f);
    chara->animator->getLayer(2).weight = std::max(-cosf(chara->velo * gfxm::pi), 0.0f);

    gfxm::vec3 pos = chara->transform->worldPosition();

    gfxm::mat4 sweep_from = 
        gfxm::translate(
            gfxm::mat4(1.0f), 
            chara->prev_pos + gfxm::vec3(.0f, 1.0f, .0f)
        );
    gfxm::mat4 sweep_to = 
        gfxm::translate(
            gfxm::mat4(1.0f), 
            pos + gfxm::vec3(.0f, 1.0f, .0f)
        );

    gfxm::vec3 sweep_normal;
    if(chara->physics_world->sphereSweep(
        0.5f, 
        sweep_from,
        sweep_to,
        sweep_normal
    )) {
        gfxm::vec3 delta = pos - chara->prev_pos;
        float delta_len = gfxm::length(delta);
        sweep_normal = gfxm::normalize(sweep_normal);
        sweep_normal *= delta_len;
        chara->transform->translate(sweep_normal);
    }

    pos = chara->transform->worldPosition();
    gfxm::vec3 hit; 
    if(chara->physics_world->rayCastClosestPoint(
        gfxm::ray(pos + gfxm::vec3(.0f, .5f, .0f), gfxm::vec3(.0f, -.7f, .0f)),
        hit
    )) {
        chara->transform->position(gfxm::lerp(pos, hit, 0.2f));    
    } else {
        chara->transform->translate(gfxm::vec3(.0f, -10.0f, .0f) * (1.0f / 60.0f));
    }

    DebugDraw::getInstance()->line(pos + gfxm::vec3(.0f, .5f, .0f), pos + gfxm::vec3(.0f, -.7f, .0f), gfxm::vec3(1.0f, .0f, .0f));
}

#endif
