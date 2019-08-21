#include "action_state_machine.hpp"

#include "../util/imgui_helpers.hpp"

#include <algorithm>

void ActionStateMachine::update(float dt) {
    if(skeleton_nodes_dirty) {
        if(skeleton) {
            skeleton_nodes.resize(skeleton->boneCount());
            for(size_t i = 0; i < skeleton->boneCount(); ++i) {
                auto& bone = skeleton->getBone(i);
                ktNode* node = getOwner()->findObject(bone.name);
                skeleton_nodes[i] = node;
            }
        }
        skeleton_nodes_dirty = true;
    }

    for(auto n : skeleton_nodes) {
        if(!n) continue;
        n->getTransform()->rotate(1.0f/60.0f, gfxm::vec3(1,0,0));
    }
}

void ActionStateMachine::onGui() {
    imguiResourceTreeCombo("skeleton", skeleton, "skl", [this](){
        skeleton_nodes_dirty = true;
    });
    imguiResourceTreeCombo("graph", graph, "action_graph", [](){

    });

    static int selected_anim_mapping = 0;
    if(ImGui::ListBoxHeader("animations")) {
        for(size_t i = 0; i < anim_mappings.size(); ++i) {
            if(ImGui::Selectable(anim_mappings[i].alias.c_str(), selected_anim_mapping == i)) {
                selected_anim_mapping = i;
            }
        }

        ImGui::ListBoxFooter();
    }
    if(ImGui::SmallButton(ICON_MDI_PLUS)) {
        anim_mappings.emplace_back(
            AnimMapping{ "Anim alias" }
        );
        selected_anim_mapping = anim_mappings.size() - 1;
    }
    ImGui::SameLine();
    if(ImGui::SmallButton(ICON_MDI_MINUS)) {
        if(selected_anim_mapping < anim_mappings.size()) {
            anim_mappings.erase(anim_mappings.begin() + selected_anim_mapping);
        }
    }

    if(selected_anim_mapping < anim_mappings.size()) {
        auto& mapping = anim_mappings[selected_anim_mapping];
        std::shared_ptr<Animation> anim;
        imguiResourceTreeCombo("anim", mapping.anim, "anm", [](){
            // TODO: Update mapping
        });
        char buf[256];
        memset(buf, 0, sizeof(buf));
        memcpy(buf, mapping.alias.data(), std::min((int)mapping.alias.size(), 256));
        if(ImGui::InputText("alias", buf, sizeof(buf))) {
            mapping.alias = buf;
        }
    }
}

void ActionStateMachine::write(SceneWriteCtx& out) {
    out.write(skeleton);
} 
void ActionStateMachine::read(SceneReadCtx& in) {
    skeleton = in.readResource<Skeleton>();
}