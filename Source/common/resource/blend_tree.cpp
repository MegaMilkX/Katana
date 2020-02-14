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

int BlendTree::getValueIndex(const char* name) {
    auto it = value_indices.find(name);
    if(it == value_indices.end()) {
        return -1;
    }
    return it->second;
}
const char* BlendTree::getValueName(int idx) {
    for(auto& kv : value_indices) {
        if(kv.second == idx) {
            return kv.first.c_str();
        }
    }
    return "";
}
int BlendTree::declValue(const char* name) {
    int idx = getValueIndex(name);
    if(idx >= 0) {
        LOG_WARN("BlendTree value " << std::string(name) << " already declared");
        return idx;
    }

    idx = (int)values.size();
    value_indices[name] = idx;
    values.emplace_back(.0f);
    return idx;
}
void BlendTree::removeValue(const char* name) {
    int idx = getValueIndex(name);
    if(idx < 0) {
        LOG_WARN("Failed to remove BlendTree value " << std::string(name) << ": doesn't exist");
        return;
    }

    values.erase(values.begin() + idx);
    value_indices.erase(name);
    
    for(auto& kv : value_indices) {
        if(kv.second > idx) {
            kv.second--;
        }
    }
}
void BlendTree::setValue(int idx, float val) {
    values[idx] = val;
}
float BlendTree::getValue(int idx) {
    return values[idx];
}
int BlendTree::valueCount() {
    return (int)values.size();
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

    JobGraph::clear();
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