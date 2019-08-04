#ifndef ANIM_CONTROLLER_HPP
#define ANIM_CONTROLLER_HPP

#include "../scene_controller.hpp"
#include "../game_scene.hpp"
#include "../../components/model.hpp"

#include "../../components/animation_stack.hpp"

class AnimController : public SceneEventFilter<AnimationStack> {
    RTTR_ENABLE(SceneController)
public:
    virtual void onAttribCreated(AnimationStack* s) { stacks.insert(s); }
    virtual void onAttribRemoved(AnimationStack* s) { stacks.erase(s); }

    virtual SceneCtrlInfo getInfo() const {
        return SceneCtrlInfo{ true, FRAME_PRIORITY_ANIM };
    }

    virtual void onUpdate() {
        for(auto s : stacks) {
            s->update(1.0f/60.0f);
        }
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
