#ifndef CONSTRAINT_COLLIDE_HPP
#define CONSTRAINT_COLLIDE_HPP

#include "constraint.hpp"

#include "../../scene/game_object.hpp"

#include "../../../common/util/imgui_helpers.hpp"

#include "../../scene/controllers/dynamics_ctrl.hpp"

#include "../../components/collider.hpp"

namespace Constraint {

class Collide : public Constraint {
    RTTR_ENABLE(Constraint)
public:
    virtual void copy(Constraint* other) {
        Collide* o = (Collide*)other;
        weight = o->weight;
    }

    virtual void update(GameObject* t) {
        gfxm::vec3 new_pos = t->getTransform()->getWorldPosition();
        auto col = t->find<Collider>();
        t->getScene()->getController<DynamicsCtrl>()->getAdjustedPosition(col.get(), new_pos, 1);
        t->getTransform()->setPosition(new_pos);
    }
    virtual void onGui() {

    }

    virtual void serialize(out_stream& out) {
        out.write(weight);
    }
    virtual void deserialize(in_stream& in) {
        weight = in.read<float>();
    }
private:
    float weight = 1.0f;
    GameObject* obj = 0;
};

}

#endif
