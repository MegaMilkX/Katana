#include "rigid_body.hpp"

#include "../scene/controllers/dynamics_ctrl.hpp"

void RigidBody::onCreate() {}

static void checkboxHelper(int i, const char* postfix, uint32_t& tgt, int bit) {
    ImGui::CheckboxFlags(
        MKSTR(i << postfix).c_str(),
        &tgt,
        1 << bit
    );
}

void RigidBody::onGui() {
    ImGui::Text("Collision group");
    uint32_t flags = getCollisionGroup();
    checkboxHelper(0, MKSTR("##" << this).c_str(), flags, 0);
    ImGui::SameLine();
    checkboxHelper(1, MKSTR("##" << this).c_str(), flags, 1);
    checkboxHelper(2, MKSTR("##" << this).c_str(), flags, 2);
    ImGui::SameLine();
    checkboxHelper(3, MKSTR("##" << this).c_str(), flags, 3);
    checkboxHelper(4, MKSTR("##" << this).c_str(), flags, 4);
    ImGui::SameLine();
    checkboxHelper(5, MKSTR("##" << this).c_str(), flags, 5);
    if(flags != getCollisionGroup()) {
        setCollisionGroup(flags);
    }

    ImGui::Text("Collides with");
    flags = getCollisionMask();
    checkboxHelper(0, MKSTR("##mask" << this).c_str(), flags, 0);
    ImGui::SameLine();
    checkboxHelper(1, MKSTR("##mask" << this).c_str(), flags, 1);
    checkboxHelper(2, MKSTR("##mask" << this).c_str(), flags, 2);
    ImGui::SameLine();
    checkboxHelper(3, MKSTR("##mask" << this).c_str(), flags, 3);
    checkboxHelper(4, MKSTR("##mask" << this).c_str(), flags, 4);
    ImGui::SameLine();
    checkboxHelper(5, MKSTR("##mask" << this).c_str(), flags, 5);
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