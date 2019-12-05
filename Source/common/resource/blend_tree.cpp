#include "blend_tree.hpp"

void BlendTree::clear() {
    jobGraph.clear();
    poseResult = new PoseResultJob;
    jobGraph.addNode(poseResult);
}
void BlendTree::copy(BlendTree* other) {
    dstream strm;
    other->serialize(strm);
    strm.jump(0);
    deserialize(strm, strm.bytes_available());
}

void BlendTree::setSkeleton(std::shared_ptr<Skeleton> skel) {
    for(auto a : anim_nodes) {
        a->setSkeleton(skel);
    }
}

void BlendTree::setBlackboard(AnimBlackboard* bb) {
    blackboard = bb;
    // TODO: Hook up value nodes
}

void BlendTree::serialize(out_stream& out) {
    DataWriter w(&out);

    jobGraph.write(out);
    out.write(poseResult->getUid());
    out.write<uint32_t>(anim_nodes.size());
    for(auto a : anim_nodes) {
        out.write<uint32_t>(a->getUid());
    }
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

    anim_nodes.clear();

    jobGraph.clear();
    jobGraph.read(in);
    uint32_t pose_node_uid = r.read<uint32_t>();
    poseResult = (PoseResultJob*)jobGraph.getNode(pose_node_uid);
    uint32_t anim_node_uid_count = r.read<uint32_t>();
    for(uint32_t i = 0; i < anim_node_uid_count; ++i) {
        auto node = (SingleAnimJob*)jobGraph.getNode(r.read<uint32_t>());
        anim_nodes.insert(node);
        node->setBlendTree(this);
    }
    std::string ref_name = r.readStr();
    std::string skl_name = r.readStr();
    ref_object = retrieve<GameScene>(ref_name);
    ref_skel = retrieve<Skeleton>(skl_name);

    jobGraph.prepare();
    return true;
}