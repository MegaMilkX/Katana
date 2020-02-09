#include "action_state_machine.hpp"

#include "../util/imgui_helpers.hpp"

#include <algorithm>


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
        graph.setSkeleton(skeleton);
        graph.setBlackboard(&blackboard);
    }
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
    
    graph.update(dt, sample_buffer);

    assert(skeleton_nodes.size() == sample_buffer.size());
    for(size_t i = 0; i < skeleton_nodes.size(); ++i) {
        auto n = skeleton_nodes[i];
        if(!n) continue;
        n->getTransform()->setPosition(sample_buffer[i].t);
        n->getTransform()->setRotation(sample_buffer[i].r);
        n->getTransform()->setScale(sample_buffer[i].s);
    }
}

void ActionStateMachine::onGui() {
    imguiResourceTreeCombo("skeleton", skeleton, "skl", [this](){
        skeleton_nodes_dirty = true;
        resizeSampleBuffer();
        graph.setSkeleton(skeleton);
    });
    imguiResourceTreeCombo("graph", graph_ref, "action_graph", [this](){
        makeGraphLocalCopy();
    });

    for(size_t i = 0; i < blackboard.count(); ++i) {
        float f = blackboard.get_float(i);
        if(ImGui::DragFloat(blackboard.getName(i), &f, 0.001f)) {
            blackboard.set(i, f);
        }
    }
}

void ActionStateMachine::write(SceneWriteCtx& out) {
    out.write(skeleton);
    out.write(graph_ref);
} 
void ActionStateMachine::read(SceneReadCtx& in) {
    skeleton = in.readResource<Skeleton>();
    graph_ref = in.readResource<AnimFSM>();

    resizeSampleBuffer();
    makeGraphLocalCopy();
}
