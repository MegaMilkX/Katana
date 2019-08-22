#include "action_state_machine.hpp"

#include "../util/imgui_helpers.hpp"

#include <algorithm>

void buildAnimSkeletonMapping(Animation* anim, Skeleton* skel, std::vector<int32_t>& bone_mapping) {
    if(!anim || !skel) return;

    bone_mapping = std::vector<int32_t>(anim->nodeCount(), -1);
    for(size_t i = 0; i < skel->boneCount(); ++i) {
        auto& bone = skel->getBone(i);
        int32_t bone_index = (int32_t)i;
        int32_t node_index = anim->getNodeIndex(bone.name);
        if(node_index < 0) continue;
        
        bone_mapping[node_index] = bone_index;
    }
}

void ActionStateMachine::buildAnimSkeletonMappings() {
    for(auto& m : anim_mappings) {
        buildAnimSkeletonMapping(m.anim.get(), skeleton.get(), m.bone_mapping);
    }
}
void ActionStateMachine::resizeSampleBuffer() {
    if(!skeleton) {
        sample_buffer.clear();
        return;
    }
    sample_buffer.resize(skeleton->boneCount());
}

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

    static float cursor = .0f;
    if(anim_mappings.size()) {
        auto& mapping = anim_mappings[0];
        if(mapping.anim) {
            cursor += dt * mapping.anim->fps;
            if(cursor > mapping.anim->length) {
                cursor -= mapping.anim->length;
            }
            mapping.anim->sample_remapped(
                sample_buffer,
                cursor,
                mapping.bone_mapping
            );
        }
    }

    assert(skeleton_nodes.size() == sample_buffer.size());
    for(size_t i = 0; i < skeleton_nodes.size(); ++i) {
        auto n = skeleton_nodes[i];
        if(!n) continue;
        n->getTransform()->setPosition(sample_buffer[i].t);
        n->getTransform()->setRotation(sample_buffer[i].r);
        n->getTransform()->setScale(sample_buffer[i].s);
    }
/*
    for(auto n : skeleton_nodes) {
        if(!n) continue;
        n->getTransform()->rotate(1.0f/60.0f, gfxm::vec3(1,0,0));
    }*/
}

void ActionStateMachine::onGui() {
    imguiResourceTreeCombo("skeleton", skeleton, "skl", [this](){
        skeleton_nodes_dirty = true;
        buildAnimSkeletonMappings();
        resizeSampleBuffer();
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
        imguiResourceTreeCombo("anim", mapping.anim, "anm", [this, &mapping](){
            buildAnimSkeletonMapping(mapping.anim.get(), skeleton.get(), mapping.bone_mapping);
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
    out.write(graph);
    out.write<uint32_t>(anim_mappings.size());
    for(size_t i = 0; i < anim_mappings.size(); ++i) {
        auto& mapping = anim_mappings[i];
        out.write(mapping.alias);
        out.write(mapping.anim);
        out.write(mapping.bone_mapping);
    }
} 
void ActionStateMachine::read(SceneReadCtx& in) {
    skeleton = in.readResource<Skeleton>();
    graph = in.readResource<ActionGraph>();
    uint32_t mapping_count = in.read<uint32_t>();
    anim_mappings.resize(mapping_count);
    for(uint32_t i = 0; i < mapping_count; ++i) {
        auto& mapping = anim_mappings[i];
        mapping.alias = in.readStr();
        mapping.anim = in.readResource<Animation>();
        mapping.bone_mapping = in.readArray<int32_t>();
    }

    buildAnimSkeletonMappings();
    resizeSampleBuffer();
}
