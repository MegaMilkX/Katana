#ifndef ANIM_CONTROLLER_HPP
#define ANIM_CONTROLLER_HPP

#include "../scene_controller.hpp"
#include "../game_scene.hpp"
#include "../../components/model.hpp"

#include "../../components/animation_stack.hpp"

class AnimController : public SceneController {
    RTTR_ENABLE(SceneController)
public:
    virtual SceneCtrlInfo getInfo() const {
        return SceneCtrlInfo{ true, FRAME_PRIORITY_ANIM };
    }

    virtual void onUpdate() {
        for(auto s : stacks) {
            s->update(1.0f/60.0f);
        }
    }

    void _regStack(AnimationStack* s) {
        stacks.insert(s);
    }
    void _unregStack(AnimationStack* s) {
        stacks.erase(s);
    }
private:
    std::set<AnimationStack*> stacks;
};
STATIC_RUN(AnimController) {
    rttr::registration::class_<AnimController>("AnimController")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
