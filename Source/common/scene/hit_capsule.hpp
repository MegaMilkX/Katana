#ifndef HIT_CAPSULE_HPP
#define HIT_CAPSULE_HPP

#include "stateful_actor.hpp"


#include "../attributes/animation_stack.hpp"
#include "../../common/resource/audio_clip.hpp"
#include "../../common/audio.hpp"

class HitCapsule 
: public StatefulActor {
public:
    virtual void init() {
        anim_stack = get<AnimationStack>().get();
        clip_hit = retrieve<AudioClip>("assets/audio/hit.ogg");

        addState("idle", [this](){
            anim_stack->blendOverFromCurrentPose(.1f);
            anim_stack->play(0, "Idle");
        }, [this](){

        });
        addState("hit", [this](){
            anim_stack->blendOverFromCurrentPose(.1f);
            anim_stack->play(0, "Hit");
            hit = false;
            audio().playOnce3d(clip_hit->getBuffer(), getTransform()->getWorldPosition());
        }, [this](){

        });
        addTransition("idle", "hit", [this]()->bool{
            return hit;
        });
        addTransition("hit", "idle", [this]()->bool{
            return anim_stack->layerStopped(0);
        });
    }
    virtual void reset() {
        switchState("idle");
    }

    void onHit() {
        hit = true;
    }
private:
    AnimationStack* anim_stack;
    std::shared_ptr<AudioClip> clip_hit;
    bool hit = false;
};

#endif