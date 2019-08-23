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
    if(!skeleton) {
        return;
    }
    if(!graph_ref) {
        return;
    }

    graph_ref->makeMappings(skeleton.get(), mappings);
}
void ActionStateMachine::resizeSampleBuffer() {
    if(!skeleton) {
        sample_buffer.clear();
        return;
    }
    sample_buffer.resize(skeleton->boneCount());
    for(auto& s : sample_buffer) {
        s.r = gfxm::quat(.0f, .0f, .0f, 1.0f);
        s.s = gfxm::vec3(1.0f, 1.0f, 1.0f);
    }
}
void ActionStateMachine::makeGraphLocalCopy() {
    if(graph_ref) {
        dstream strm;
        graph_ref->serialize(strm);
        strm.jump(0);
        graph.deserialize(strm, strm.bytes_available());
    }
    buildAnimSkeletonMappings();
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
    if(!graph_ref) {
        return;
    }
    
    graph.update(dt, sample_buffer, mappings);

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
    imguiResourceTreeCombo("graph", graph_ref, "action_graph", [this](){
        makeGraphLocalCopy();
    });
}

void ActionStateMachine::write(SceneWriteCtx& out) {
    out.write(skeleton);
    out.write(graph_ref);
} 
void ActionStateMachine::read(SceneReadCtx& in) {
    skeleton = in.readResource<Skeleton>();
    graph_ref = in.readResource<ActionGraph>();

    resizeSampleBuffer();
    makeGraphLocalCopy();
}
