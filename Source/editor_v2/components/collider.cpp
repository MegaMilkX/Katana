#include "collider.hpp"

#include "../scene/controllers/dynamics_ctrl.hpp"

void Collider::onCreate() {
    getOwner()->getScene()->getController<DynamicsCtrl>();
}

static void checkboxHelper(DynamicsCtrl* ctrl, const char* postfix, uint32_t& tgt, int bit) {
    ImGui::CheckboxFlags(
        (ctrl->getCollisionGroupName((COLLISION_GROUP)((int)COLLISION_GROUP_0 + bit)) + postfix).c_str(),
        &tgt,
        1 << bit
    );
}

void Collider::onGui() {
    auto dctrl = getOwner()->getScene()->getController<DynamicsCtrl>();

    if(ImGui::Checkbox("Is Ghost", &is_ghost)) {
        setGhost(is_ghost);
    }

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

    gfxm::vec3 offset_ = getOffset();
    if(ImGui::DragFloat3Autospeed(MKSTR("offset##" << this).c_str(), (float*)&offset_)) {
        setOffset(offset_);
    }
}