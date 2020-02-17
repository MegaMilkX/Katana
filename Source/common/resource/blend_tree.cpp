#include "blend_tree.hpp"

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