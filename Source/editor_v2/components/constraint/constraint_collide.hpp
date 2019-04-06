#ifndef CONSTRAINT_COLLIDE_HPP
#define CONSTRAINT_COLLIDE_HPP

#include "constraint.hpp"

#include "../../scene/game_object.hpp"

#include "../../../common/util/imgui_helpers.hpp"

#include "../../scene/controllers/dynamics_ctrl.hpp"
#include "../collision_shape.hpp"

#include "../ghost_object.hpp"
#include "../collision_shape.hpp"

namespace Constraint {

class Collide : public Constraint {
    RTTR_ENABLE(Constraint)
public:
    virtual void copy(Constraint* other) {
        Collide* o = (Collide*)other;
        weight = o->weight;
    }

    virtual void update(GameObject* t) {
        if(!obj) {
            obj = t;
            t_prev = t->get<GhostObject>()->getBtObject()->getWorldTransform();
            t->get<CmCollisionShape>()->getOffset();
        }
        btTransform trans = t->get<GhostObject>()->getBtObject()->getWorldTransform();
        gfxm::mat4 mf;
        gfxm::mat4 mt;
        t_prev.getOpenGLMatrix((float*)&mf);
        trans.getOpenGLMatrix((float*)&mt);
        //gfxm::vec3 new_pos = t->get<GhostObject>()->sweep(mf, mt);

        gfxm::vec3 new_pos = t->get<GhostObject>()->getCollisionAdjustedPosition();
        t->getTransform()->setPosition(new_pos);
        t_prev = trans;
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
    btTransform t_prev;
};

}

#endif
