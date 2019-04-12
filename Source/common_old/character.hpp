#ifndef CHARACTER_HPP
#define CHARACTER_HPP

#include "state_machine.hpp"
#include "component.hpp"
#include "gfxm.hpp"
#include "behavior_system.hpp"
#include "input/input_mgr.hpp"

#include "transform.hpp"
#include "animator.hpp"
#include "components/collision_beacon.hpp"

#include "scene_components/scene_player_info.hpp"
#include "scene_components/scene_physics_world.hpp"
#include "scene.hpp"

#include "debug_draw.hpp"

class Character : public Updatable {
    CLONEABLE
    RTTR_ENABLE(Updatable)
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
        sensor = get<CapsuleBeacon>();
        physics_world = getScene()->getSceneComponent<PhysicsWorld>();

        ScenePlayerInfo* pi = getScene()->getSceneComponent<ScenePlayerInfo>();
        pi->character_transform = transform;
    }

    void onClone(Character* other) {

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
        input_lis->bindActionPress(
            "Attack",
            [this]() {
                attacking = true;
            }
        );
        
        motion_fsm = StateMachine<Character>(this);

        motion_fsm.createState(
            "idle",
            [this](Character* chara){
                velo = 0.0f;

                chara->animator->blendOver(0, "Idle");
                chara->animator->getLayer(0).weight = 1.0f;
                chara->animator->getLayer(1).weight = 0.0f;
                chara->animator->getLayer(2).weight = 0.0f;

                chara->animator->getLayer(0).mode = ANIM_MODE_NONE;
                chara->animator->getLayer(1).mode = ANIM_MODE_BLEND;
                chara->animator->getLayer(2).mode = ANIM_MODE_BLEND;
            },
            [this](Character* chara){

            }
        );
        motion_fsm.createState(
            "run",
            [this](Character* chara){
                chara->animator->blendOver(0, "Idle");
                chara->animator->play(1, "Walk");
                chara->animator->play(2, "Run");
            },
            [this](Character* chara){
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
                gfxm::vec3 displacement = pos - chara->prev_pos;

                gfxm::vec3 corrected_position = pos;
                if(chara->sensor->contactTest(corrected_position, 1)) {
                    chara->transform->position(corrected_position);
                }
            }
        );
        motion_fsm.createState(
            "fall",
            [this](Character* chara){
                chara->animator->blendOver(0, "Fall");
                chara->animator->getLayer(0).weight = 1.0f;
                chara->animator->getLayer(1).weight = 0.0f;
                chara->animator->getLayer(2).weight = 0.0f;
            },
            [this](Character* chara){
                chara->transform->translate(gfxm::vec3(.0f, -10.0f, .0f) * (1.0f / 60.0f));
            }
        );
        motion_fsm.createState(
            "kick",
            [this](Character* chara) {
                attacking = false;
                chara->animator->blendOver(0, "Kick");
                chara->animator->getLayer(0).weight = 1.0f;
                chara->animator->getLayer(1).weight = 0.0f;
                chara->animator->getLayer(2).weight = 0.0f;
            },
            [this](Character* chara) {

            }
        );
        motion_fsm.addTransition("idle", "run", [](Character* chara)->bool{
            if(chara->velo > 0.0001f) {
                return true;
            }
            return false;
        });
        motion_fsm.addTransition("run", "idle", [](Character* chara)->bool{
            if(chara->velo < 0.0001f) {
                return true;
            }
            return false;
        });
        motion_fsm.addTransition("run", "fall", [](Character* chara)->bool{
            if(!chara->is_grounded) {
                return true;
            }
            return false;
        });
        motion_fsm.addTransition("idle", "fall", [](Character* chara)->bool{
            if(!chara->is_grounded) {
                return true;
            }
            return false;
        });
        motion_fsm.addTransition("fall", "idle", [](Character* chara)->bool{
            if(chara->is_grounded) {
                return true;
            }
            return false;
        });
        motion_fsm.addTransition("idle", "kick", [this](Character* chara)->bool{
            if(is_grounded && attacking) {
                return true;
            }
            return false;
        });
        motion_fsm.addTransition("run", "kick", [this](Character* chara)->bool{
            if(is_grounded && attacking) {
                return true;
            }
            return false;
        });
        motion_fsm.addTransition("kick", "idle", [this](Character* chara)->bool{
            if(animator->layerStopped(0)) {
                return true;
            }
            return false;
        });

        LOG("Done");
    }

    virtual void update() {
        direction = gfxm::normalize(direction);
        velo = gfxm::lerp(velo, gfxm::length(direction), 0.05f);
        motion_fsm.update();
        prev_pos = transform->worldPosition();

        gfxm::vec3 pos = transform->worldPosition();
        gfxm::vec3 hit; 
        /*
        if(physics_world->rayCastClosestPoint(
            gfxm::ray(pos + gfxm::vec3(.0f, .5f, .0f), gfxm::vec3(.0f, -.7f, .0f)),
            hit
        )) {
            transform->position(gfxm::lerp(pos, hit, 0.2f));
            is_grounded = true;    
        } else {
            is_grounded = false;
        }*/
        if(physics_world->sphereSweepClosestHit(
            0.2f, 
            pos + gfxm::vec3(.0f, 1.0f, .0f),
            pos + gfxm::vec3(.0f, -0.5f, .0f),
            1,
            hit
        )) {
            transform->position(gfxm::lerp(pos, hit, 0.2f));
            is_grounded = true;    
        } else {
            is_grounded = false;
        }

        sensor->updateTransform();

        DebugDraw::getInstance()->line(pos + gfxm::vec3(.0f, .5f, .0f), pos + gfxm::vec3(.0f, -.7f, .0f), gfxm::vec3(1.0f, .0f, .0f));
    }

    virtual void cleanup() {
        input().removeListener(input_lis);
    }

    gfxm::vec2 direction;
    bool is_grounded = true;
private:
    bool attacking = false;

    float velo = 0.0f;
    gfxm::vec3 prev_pos;

    Animator* animator = 0;
    Transform* transform = 0;
    CapsuleBeacon* sensor = 0;
    PhysicsWorld* physics_world = 0;

    InputListener* input_lis;

    StateMachine<Character> motion_fsm;
};
STATIC_RUN(Character)
{
    rttr::registration::class_<Character>("Character")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
