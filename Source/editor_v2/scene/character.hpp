#ifndef CHARACTER_HPP
#define CHARACTER_HPP

#include "actor_object.hpp"

#include "../input_listener.hpp"

#include "game_scene.hpp"

#include <list>

class StatefulActor : public ActorObject {
    RTTR_ENABLE(ActorObject)
public:
    typedef std::function<void(void)> state_init_cb_t;
    typedef std::function<void(void)> state_update_cb_t;
    typedef std::function<bool(void)> trans_cond_fn_t;

    struct State;
    struct Transition {
        trans_cond_fn_t cond_func;
        std::string target;
    };
    struct State {
        state_init_cb_t init_func;
        state_update_cb_t update_func;
        std::vector<Transition> transitions;
    };

    void addState(
        const std::string& name,
        state_init_cb_t init_func,
        state_update_cb_t update_func
    ) {
        auto ptr = std::shared_ptr<State>(new State{init_func, update_func});
        states[name] = ptr;
    }
    void addTransition(
        const std::string& origin, 
        const std::string& target,
        trans_cond_fn_t func
    ) {
        auto& it = states.find(origin);
        if(it != states.end()) {
            it->second->transitions.emplace_back(Transition{func, target});
        }
    }
    void addTransitions(
        const std::vector<std::string>& origins,
        const std::string& target,
        trans_cond_fn_t func
    ) {
        for(auto& o : origins) {
            addTransition(o, target, func);
        }
    }

    void switchState(const std::string& target) {
        auto& it = states.find(target);
        if(it != states.end()) {
            it->second->init_func();
            current_state = it->second.get();
        } else {
            LOG("State " << target << " not found");
        }
    }

    virtual void update() {
        if(!current_state) return;
        for(auto& t : current_state->transitions) {
            if(t.cond_func()) {
                switchState(t.target);
            }
        }
        if(!current_state) return;
        current_state->update_func();
    }
private:
    std::map<std::string, std::shared_ptr<State>> states;
    State* current_state = 0;
};

#include "../components/animation_stack.hpp"

class CharacterActor 
: public StatefulActor, public InputListenerWrap
{
    RTTR_ENABLE(StatefulActor)
public:
    virtual void init() {
        anim_stack = get<AnimationStack>().get();
        
        bindAxis("MoveHori", [this](float v){
            desired_direction.x = v;
        });
        bindAxis("MoveVert", [this](float v){
            desired_direction.z = -v;
        });
        bindActionPress("Attack", [this](){
            attacking = true;
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
                if(getScene()->getDefaultCamera()) {
                    cam_transform = getScene()->getDefaultCamera()->getOwner()->getTransform()->getWorldTransform();
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
    }
    virtual void reset() {
        switchState("idle");
    }
    virtual void update() {
        StatefulActor::update();

        if(gfxm::length(desired_direction) > 0.01f) {
            velocity = gfxm::lerp(velocity, 1.0f, 0.05f);
        } else {
            velocity = gfxm::lerp(velocity, 0.0f, 0.05f);
        }
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

    float velocity = 0.0f;

    gfxm::vec3 desired_direction;

    bool attacking = false;
};
STATIC_RUN(CharacterActor) {
    rttr::registration::class_<CharacterActor>("CharacterActor")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
