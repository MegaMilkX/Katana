#ifndef CHARACTER_HPP
#define CHARACTER_HPP

#include "../input_listener.hpp"

#include "game_scene.hpp"

#include <list>

#include "controllers/render_controller.hpp"
#include "controllers/dynamics_ctrl.hpp"

#include "stateful_actor.hpp"

#include "../attributes/animation_stack.hpp"
#include "../../common/resource/audio_clip.hpp"
#include "../../common/audio.hpp"

class CharacterActor 
: public StatefulActor, public InputListenerWrap
{
public:
    std::shared_ptr<AudioClip> footstep_clips[4];
    std::shared_ptr<AudioClip> clip_swoosh;

    void playRandomFootstep() {
        auto& clip = footstep_clips[rand() % 4];
        audio().playOnce3d(clip->getBuffer(), getTransform()->getWorldPosition());
    }
    virtual void init() {
        anim_stack = get<AnimationStack>().get();

        footstep_clips[0] = retrieve<AudioClip>("assets/audio/gravel1.ogg");
        footstep_clips[1] = retrieve<AudioClip>("assets/audio/gravel2.ogg");
        footstep_clips[2] = retrieve<AudioClip>("assets/audio/gravel3.ogg");
        footstep_clips[3] = retrieve<AudioClip>("assets/audio/gravel4.ogg");
        
        clip_swoosh = retrieve<AudioClip>("assets/audio/swoosh.ogg");
        anim_stack->setEventCallback("footstep.L", [this](){
            playRandomFootstep();
        });
        anim_stack->setEventCallback("footstep.R", [this](){
            playRandomFootstep();
        });
        anim_stack->setEventCallback("swoosh", [this](){
            audio().playOnce3d(clip_swoosh->getBuffer(), getTransform()->getWorldPosition());
        });
        
        bindAxis("MoveHori", [this](float v){
            desired_direction.x = v;
        });
        bindAxis("MoveVert", [this](float v){
            desired_direction.z = -v;
        });
        bindActionPress("Attack", [this](){
            attacking = true;
        });
        bindActionPress("SlowWalk", [this](){
            walk_mod = 0.5f;
        });
        bindActionRelease("SlowWalk", [this](){
            walk_mod = 1.0f;
        });

        addState("idle", [this](){
            velocity = .0f;
            attacking = false;
            anim_stack->blendOverFromCurrentPose(.24f);
            anim_stack->play(0, "Idle");
            anim_stack->getLayer(0).weight = 1.0f;
            anim_stack->getLayer(1).weight = 0.0f;
            anim_stack->getLayer(2).weight = 0.0f;
            anim_stack->getLayer(0).mode = ANIM_MODE_NONE;
        }, [this](){
            
        });
        addState("run", [this](){
            anim_stack->blendOverFromCurrentPose(.15f);
            anim_stack->play(0, "Idle");
            anim_stack->play(1, "Walk");
            anim_stack->play(2, "Run");
            anim_stack->getLayer(0).mode = ANIM_MODE_NONE;
            anim_stack->getLayer(1).mode = ANIM_MODE_BLEND;
            anim_stack->getLayer(2).mode = ANIM_MODE_BLEND;
            anim_stack->getLayer(1).cursor = .0f;
            anim_stack->getLayer(2).cursor = .0f;
        }, [this](){
            if(gfxm::length(desired_direction) > 0.01f) {
                gfxm::mat4 cam_transform = gfxm::mat4(1.0f);
                auto cam = getScene()->getController<RenderController>()->getDefaultCamera();
                if(cam) {
                    cam_transform = cam->getOwner()->getTransform()->getWorldTransform();
                }
                gfxm::vec4 dir = cam_transform * gfxm::vec4(desired_direction, .0f);
                getTransform()->lookAt(
                    getTransform()->getWorldPosition() +
                    gfxm::vec3(dir.x, .0f, dir.z),
                    gfxm::vec3(.0f, 1.0f, .0f),
                    0.1f
                );
            }

            anim_stack->getLayer(0).weight = 1.0f;
            anim_stack->getLayer(1).weight = std::max(sinf(velocity * gfxm::pi), 0.0f);
            anim_stack->getLayer(2).weight = std::max(-cosf(velocity * gfxm::pi), 0.0f);
            anim_stack->getLayer(1).speed = gfxm::lerp(
                1.0f,
                anim_stack->getLengthProportion(1, 2),
                anim_stack->getLayer(2).weight
            );
            anim_stack->getLayer(2).speed = gfxm::lerp(
                anim_stack->getLengthProportion(2, 1),
                1.0f,
                anim_stack->getLayer(2).weight
            );
        });
        addState("attack", [this](){
            anim_stack->blendOverFromCurrentPose(.1f);
            anim_stack->play(0, "Attack");
            anim_stack->getLayer(0).weight = 1.0f;
            anim_stack->getLayer(1).weight = 0.0f;
            anim_stack->getLayer(2).weight = 0.0f;
        }, [this](){
            float v = anim_stack->getCurveValue("Hitbox.Active");
            if(v > 0.5f) {
                LOG("HITBOX_ACTIVE");
            }
        });
        addState("fall", [this](){
            anim_stack->blendOverFromCurrentPose(.1f);
            anim_stack->play(0, "Bind");
            anim_stack->getLayer(0).weight = 1.0f;
            anim_stack->getLayer(1).weight = 0.0f;
            anim_stack->getLayer(2).weight = 0.0f;
        }, [this](){
            getTransform()->translate(gfxm::vec3(.0f, -10.0f * (1.0f/60.0f), .0f));
        });

        addTransition("idle", "run", [this]()->bool{
            if(velocity > 0.01f)
                return true;
            return false;
        });
        addTransition("run", "idle", [this]()->bool{
            if(velocity < 0.01f)
                return true;
            return false;
        });
        addTransitions({"idle", "run"}, "attack", [this]()->bool{
            return attacking;
        });
        addTransition("attack", "idle", [this]()->bool{
            if(anim_stack->layerStopped(0)) {
                return true;
            }
            return false;
        });
        addTransitions({"idle", "run", "attack"}, "fall", [this]()->bool{
            if(!grounded) return true;
            return false;
        });
        addTransition("fall", "idle", [this]()->bool{
            return grounded;
        });
    }
    virtual void reset() {
        switchState("idle");
    }
    virtual void update() {
        StatefulActor::update();

        if(gfxm::length(desired_direction) > 0.01f) {
            velocity = gfxm::lerp(velocity, 1.0f * walk_mod, 0.05f);
        } else {
            velocity = gfxm::lerp(velocity, 0.0f, 0.05f);
        }

        gfxm::vec3 pos = getTransform()->getWorldPosition();
        gfxm::vec3 grnd_hit;
        float sweepRad = 0.2f;
        if(getScene()->getController<DynamicsCtrl>()
            ->sweepSphere(
                sweepRad,
                pos + gfxm::vec3(.0f, 1.0f, .0f),
                pos + gfxm::vec3(.0f,-0.5f, .0f),
                grnd_hit,
                1
            )
        ) {
            getTransform()->setPosition(gfxm::lerp(pos, grnd_hit + gfxm::vec3(.0f, -sweepRad, .0f), 0.2f));
            grounded = true;
        } else {
            grounded = false;
        }
        /*
        gfxm::vec3 new_pos = get<GhostObject>()->getCollisionAdjustedPosition();
        getTransform()->setPosition(new_pos);*/
    }

    void setCameraHeight(float v) {
        camera_height = v;
    }
    float getCameraHeight() {
        return camera_height;
    }
private:
    AnimationStack* anim_stack;

    float camera_height = 1.4f;

    bool grounded = false;

    float velocity = 0.0f;
    float walk_mod = 1.0f;

    gfxm::vec3 desired_direction;

    bool attacking = false;
};

#endif
