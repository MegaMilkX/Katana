#ifndef CONSTRAINT_SNAP_HPP
#define CONSTRAINT_SNAP_HPP

#include "constraint.hpp"

#include "../../scene/game_object.hpp"

#include "../../../common/util/imgui_helpers.hpp"

namespace Constraint {

class Snap : public Constraint {
    RTTR_ENABLE(Constraint)
public:
    virtual void copy(Constraint* other) {
        Snap* o = (Snap*)other;
        weight = o->weight;
        if(o->target) {
            target = getScene()->findObject<GameObject>(o->target->getName());
        }
    }

    virtual void update(GameObject* o) {
        auto t = o->getTransform();
        if(target) {
            auto pos = target->getTransform()->getWorldPosition();
            auto rot = target->getTransform()->getWorldRotation();
            auto src_pos = t->getWorldPosition();
            auto src_rot = t->getWorldRotation();
            t->setPosition(gfxm::lerp(src_pos, pos, weight));
            t->setRotation(gfxm::slerp(src_rot, rot, weight));
        }
    }
    virtual void onGui() {
        imguiObjectCombo(
            MKSTR("target##" << this).c_str(),
            target,
            getScene()
        );
        ImGui::DragFloat(MKSTR("weight##" << this).c_str(), &weight, 0.01f, 0.0f, 1.0f);
    }

    virtual void serialize(out_stream& out) {
        DataWriter w(&out);
        if(target) {
            w.write(target->getName());
        } else {
            w.write(std::string());
        }
        out.write(weight);
    }
    virtual void deserialize(in_stream& in) {
        DataReader r(&in);
        std::string tgt_name = r.readStr();
        weight = in.read<float>();

        target = getScene()->findObject(tgt_name);
    }
private:
    GameObject* target = 0;
    float weight = 1.0f;
};

}

#endif