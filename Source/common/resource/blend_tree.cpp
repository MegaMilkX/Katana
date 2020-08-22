#include "blend_tree.hpp"

#include "blend_tree_nodes.hpp"

BlendTree::BlendTree(Motion* motion)
: owner_motion(motion) {
    addNode(new PoseResultJob());
}

void BlendTree::clear() {
    JobGraph::clear();
    addNode(new PoseResultJob);
}
void BlendTree::copy(BlendTree* other) {
    dstream strm;
    other->write(strm);
    strm.jump(0);
    read(strm);
}

Motion* BlendTree::getMotion() {
    return owner_motion;
}
void BlendTree::rebuild() {
    JobGraph::reinitNodes();
}

void BlendTree::setCursor(float cur) {
    prev_cursor = cursor;
    cursor = cur;
    if(cursor > 1.0f) {
        cursor -= (int)cursor;
    }
}
float BlendTree::getPrevCursor() const {
    return prev_cursor;
}
float BlendTree::getCursor() const {
    return cursor;
}

Pose& BlendTree::getPoseData(float normal_cursor) {
    run();
    return pose;
}

void BlendTree::update(float dt, AnimSampleBuffer& sample_buffer) {
    run();
    prev_cursor = cursor;
    cursor += dt * pose.speed;
    if(cursor > 1.0f) {
        cursor -= 1.0f;
    }
    
    for(size_t i = 0; i < sample_buffer.sampleCount() && i < pose.sample_buffer.sampleCount(); ++i) {
        sample_buffer[i] = pose.sample_buffer[i];
    }
    sample_buffer.getRootMotionDelta() = pose.sample_buffer.getRootMotionDelta();
}

void BlendTree::write(out_stream& out) {
    DataWriter w(&out);

    JobGraph::write(out);
    
    out.write((uint32_t)0); // 
    out.write<uint32_t>(0);

    w.write(std::string());
    w.write(std::string());
}
void BlendTree::read(in_stream& in) {
    DataReader r(&in);

    JobGraph::clear();
    JobGraph::read(in);
    
    uint32_t pose_node_uid = r.read<uint32_t>(); // unused
    uint32_t anim_node_uid_count = r.read<uint32_t>(); // unused
    for(uint32_t i = 0; i < anim_node_uid_count; ++i) {
        r.read<uint32_t>(); // unused
    }
    std::string ref_name = r.readStr();
    std::string skl_name = r.readStr();

    prepare();
    rebuild();
}

void BlendTree::_reportPose(const Pose& pose) {
    this->pose = pose;
    
}