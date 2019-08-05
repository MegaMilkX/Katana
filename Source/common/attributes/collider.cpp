#include "collider.hpp"

#include "../scene/controllers/dynamics_ctrl.hpp"

void Collider::onCreate() {}

static void checkboxHelper(int i, const char* postfix, uint32_t& tgt, int bit) {
    ImGui::CheckboxFlags(
        MKSTR(i << postfix).c_str(),
        &tgt,
        1 << bit
    );
}

void Collider::onGui() {
    if(ImGui::Checkbox("Is Ghost", &is_ghost)) {
        setGhost(is_ghost);
    }

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

    imguiHeapObjectCombo(
        MKSTR("shape##" << this).c_str(),
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
    if(ImGui::ColorEdit3(MKSTR("debug color##" << this).c_str(), (float*)&debug_color)) {
        resetAttribute();
    }
}