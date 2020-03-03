#include "action_state_machine.hpp"

#include "../util/imgui_helpers.hpp"

#include <algorithm>


void ActionStateMachine::resizeSampleBuffer() {
    if(!skeleton) {
        sample_buffer = AnimSampleBuffer();
        return;
    }
    sample_buffer = AnimSampleBuffer(skeleton.get());
    for(auto& s : sample_buffer.getSamples()) {
        s.r = gfxm::quat(.0f, .0f, .0f, 1.0f);
        s.s = gfxm::vec3(1.0f, 1.0f, 1.0f);
    }
}
void ActionStateMachine::makeGraphLocalCopy() {
    if(motion_ref) {
        dstream strm;
        motion_ref->serialize(strm);
        strm.jump(0);
        motion.deserialize(strm, strm.bytes_available());
        motion.rebuild(skeleton);
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
    if(!motion_ref) {
        return;
    }
    
    motion.update(dt, sample_buffer);

    assert(skeleton_nodes.size() == sample_buffer.sampleCount());
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
        motion.rebuild(skeleton);
    });
    imguiResourceTreeCombo("graph", motion_ref, "action_graph", [this](){
        makeGraphLocalCopy();
    });
}

void ActionStateMachine::write(SceneWriteCtx& out) {
    out.write(skeleton);
    out.write(motion_ref);
} 
void ActionStateMachine::read(SceneReadCtx& in) {
    skeleton = in.readResource<Skeleton>();
    motion_ref = in.readResource<Motion>();

    resizeSampleBuffer();
    makeGraphLocalCopy();
}
