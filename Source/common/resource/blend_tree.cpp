#include "blend_tree.hpp"

void BlendTree::clear() {
    JobGraph::clear();
    addNode(new PoseResultJob);
}
void BlendTree::copy(BlendTree* other) {
    dstream strm;
    other->serialize(strm);
    strm.jump(0);
    deserialize(strm, strm.bytes_available());
}

void BlendTree::setSkeleton(std::shared_ptr<Skeleton> skel) {
    skeleton = skel;
    reinitNodes();
}

Skeleton* BlendTree::getSkeleton() {
    return skeleton.get();
}

void BlendTree::setBlackboard(AnimBlackboard* bb) {
    blackboard = bb;
    // TODO: Hook up value nodes
}

void BlendTree::serialize(out_stream& out) {
    DataWriter w(&out);

    write(out);
    
    out.write((uint32_t)0); // 
    out.write<uint32_t>(0);
    if(ref_object) {
        w.write(ref_object->Name());
    } else {
        w.write(std::string());
    }
    if(ref_skel) {
        w.write(ref_skel->Name());
    } else {
        w.write(std::string());
    }
}
bool BlendTree::deserialize(in_stream& in, size_t sz) {
    DataReader r(&in);

    clear();
    read(in);
    
    uint32_t pose_node_uid = r.read<uint32_t>(); // unused
    uint32_t anim_node_uid_count = r.read<uint32_t>(); // unused
    for(uint32_t i = 0; i < anim_node_uid_count; ++i) {
        r.read<uint32_t>(); // unused
    }
    std::string ref_name = r.readStr();
    std::string skl_name = r.readStr();
    ref_object = retrieve<GameScene>(ref_name);
    ref_skel = retrieve<Skeleton>(skl_name);

    prepare();
    return true;
}

void BlendTree::_reportPose(const Pose& pose) {
    this->pose = pose;
    
}