#ifndef CONSTRAINT_CTRL_HPP
#define CONSTRAINT_CTRL_HPP

#include "../scene_controller.hpp"
#include "../game_scene.hpp"
#include "../../attributes/constraint_stack.hpp"

class ConstraintCtrl : public SceneEventFilter<ConstraintStack> {
    RTTR_ENABLE(SceneController)
public:
    virtual void onAttribCreated(ConstraintStack* s) { stacks.insert(s); }
    virtual void onAttribRemoved(ConstraintStack* s) { stacks.erase(s); }

    virtual SceneCtrlInfo getInfo() const {
        return SceneCtrlInfo{ true, FRAME_PRIORITY_CONSTRAINT };
    }

    virtual void onUpdate() {
        for(auto s : stacks) {
            s->update();
        }
    }
private:
    std::set<ConstraintStack*> stacks;
};
STATIC_RUN(ConstraintCtrl) {
    rttr::registration::class_<ConstraintCtrl>("ConstraintCtrl")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
