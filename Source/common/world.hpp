#ifndef KT_WORLD_HPP
#define KT_WORLD_HPP

#include "scene/controllers/render_controller.hpp"
#include "scene/controllers/dynamics_ctrl.hpp"
#include "scene/controllers/anim_controller.hpp"

#include "scene/game_object.hpp"

class ktWorld : public GameObject {
    std::unique_ptr<RenderController> render_ctrl;
    std::unique_ptr<DynamicsCtrl> dynamics_ctrl;
    std::unique_ptr<AnimController> anim_ctrl;

public:
    RenderController* getRenderController() {
        return render_ctrl.get();
    }
    DynamicsCtrl* getPhysics() {
        return dynamics_ctrl.get();
    }

    void update(float dt) {
        anim_ctrl->onUpdate();
        dynamics_ctrl->onUpdate();
    }
};

#endif
