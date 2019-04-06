#ifndef CONSTRAINT_CTRL_HPP
#define CONSTRAINT_CTRL_HPP

#include "../scene_controller.hpp"
#include "../game_scene.hpp"
#include "../../components/constraint_stack.hpp"

class ConstraintCtrl : public SceneController {
    RTTR_ENABLE(SceneController)
public:
    virtual SceneCtrlInfo getInfo() const {
        return SceneCtrlInfo{ true, FRAME_PRIORITY_CONSTRAINT };
    }

    virtual void onUpdate() {
        for(auto s : stacks) {
            s->update();
        }
    }

    void _regStack(ConstraintStack* s) {
        stacks.insert(s);
    }
    void _unregStack(ConstraintStack* s) {
        stacks.erase(s);
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
