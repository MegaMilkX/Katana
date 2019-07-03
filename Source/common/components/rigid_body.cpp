#include "rigid_body.hpp"

#include "../scene/controllers/dynamics_ctrl.hpp"

void RigidBody::onCreate() {
    getOwner()->getScene()->getController<DynamicsCtrl>();
}

static void checkboxHelper(DynamicsCtrl* ctrl, const char* postfix, uint32_t& tgt, int bit) {
    ImGui::CheckboxFlags(
        (ctrl->getCollisionGroupName((COLLISION_GROUP)((int)COLLISION_GROUP_0 + bit)) + postfix).c_str(),
        &tgt,
        1 << bit
    );
}

void RigidBody::onGui() {
    auto dctrl = getOwner()->getScene()->getController<DynamicsCtrl>();

    ImGui::Text("Collision group");
    uint32_t flags = getCollisionGroup();
    checkboxHelper(dctrl, MKSTR("##" << this).c_str(), flags, 0);
    ImGui::SameLine();
    checkboxHelper(dctrl, MKSTR("##" << this).c_str(), flags, 1);
    checkboxHelper(dctrl, MKSTR("##" << this).c_str(), flags, 2);
    ImGui::SameLine();
    checkboxHelper(dctrl, MKSTR("##" << this).c_str(), flags, 3);
    checkboxHelper(dctrl, MKSTR("##" << this).c_str(), flags, 4);
    ImGui::SameLine();
    checkboxHelper(dctrl, MKSTR("##" << this).c_str(), flags, 5);
    if(flags != getCollisionGroup()) {
        setCollisionGroup(flags);
    }

    ImGui::Text("Collides with");
    flags = getCollisionMask();
    checkboxHelper(dctrl, MKSTR("##mask" << this).c_str(), flags, 0);
    ImGui::SameLine();
    checkboxHelper(dctrl, MKSTR("##mask" << this).c_str(), flags, 1);
    checkboxHelper(dctrl, MKSTR("##mask" << this).c_str(), flags, 2);
    ImGui::SameLine();
    checkboxHelper(dctrl, MKSTR("##mask" << this).c_str(), flags, 3);
    checkboxHelper(dctrl, MKSTR("##mask" << this).c_str(), flags, 4);
    ImGui::SameLine();
    checkboxHelper(dctrl, MKSTR("##mask" << this).c_str(), flags, 5);
    if(flags != getCollisionMask()) {
        setCollisionMask(flags);
    }

    float mass = getMass();
    if(ImGui::DragFloat("mass", &mass, std::max(mass * 0.01f, 0.01f))) {
        setMass(mass);
    }
    imguiHeapObjectCombo(
        "shape",
        shape,
        false,
        [this]() {
            resetAttribute();
        }
    );
    if(shape) {
        if(shape->onGui(getOwner())) {
            resetAttribute();
        }
    }
}