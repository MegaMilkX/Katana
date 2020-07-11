#include "animation.hpp"

#include "skeleton.hpp"

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

void Animation::clearNodes() {
    nodes.clear();
    node_indices.clear();
    node_names.clear();
    root_motion_node.t.set_keyframes(std::vector<keyframe<gfxm::vec3>>());
    root_motion_node.r.set_keyframes(std::vector<keyframe<gfxm::quat>>());
    root_motion_node.s.set_keyframes(std::vector<keyframe<gfxm::vec3>>());
}

size_t Animation::nodeCount() {
    return nodes.size();
}

AnimNode& Animation::getNode(const std::string& name) {
    auto it = node_indices.find(name);
    if(it == node_indices.end()) {
        node_indices[name] = nodes.size();
        node_names.emplace_back(name);
        nodes.emplace_back(AnimNode());
    }
    return nodes[node_indices[name]];
}

int32_t Animation::getNodeIndex(const std::string& name) {
    auto it = node_indices.find(name);
    if(it == node_indices.end()) {
        return -1;
    }
    return node_indices[name];
}

const std::string& Animation::getNodeName(int i) {
    return node_names[i];
}

AnimNode& Animation::getRootMotionNode() {
    return root_motion_node;
}

void Animation::setRootMotionSourceNode(const std::string& name) {
    auto it = node_indices.find(name);
    if(it == node_indices.end()) {
        root_motion_node_name.clear();
        root_motion_node_index = -1;
        return;
    }

    root_motion_node_name = name;
    root_motion_node_index = it->second;
}

bool Animation::curveExists(const std::string& name) {
    return extra_curves.count(name) != 0;
}
curve<float>& Animation::getCurve(const std::string& name) {
    return extra_curves[name];
}
size_t Animation::curveCount() const {
    return extra_curves.size();
}
curve<float>& Animation::getCurve(size_t i) {
    auto& it = extra_curves.begin();
    std::advance(it, i);
    return it->second;
}
const std::string& Animation::getCurveName(size_t i) {
    auto& it = extra_curves.begin();
    std::advance(it, i);
    return it->first;
}
void Animation::removeCurve(const std::string& name) {
    extra_curves.erase(name);
}

void Animation::setEvent(const std::string& name, float t, float threshold) {
    if(t > length) {
        t = length;
    }
    events[name] = event{t, threshold};
}
size_t Animation::eventCount() {
    return events.size();
}
std::string Animation::getEventName(size_t i) {
    auto it = events.begin();
    std::advance(it, i);
    return it->first;
}
Animation::event& Animation::getEvent(size_t i) {
    auto it = events.begin();
    std::advance(it, i);
    return it->second;
}
void Animation::removeEvent(const std::string& e) {
    events.erase(e);
}
void Animation::fireEvents(float from, float to, std::function<void(const std::string& name)> cb, float threshold) {
    for(auto& kv : events) {
        float t = kv.second.time;
        float thresh = kv.second.threshold;
        if(thresh > threshold) {
            continue;
        }
        if(from > to && t >= from) {
            cb(kv.first);
        } else if(t >= from && t < to) {
            cb(kv.first);
        }
    }
}


std::vector<int32_t>& Animation::getMapping(Skeleton* skel) {
    auto it = mappings.find(skel);
    if(it != mappings.end()) {
        return it->second;
    }
    auto& vec = mappings[skel];
    buildAnimSkeletonMapping(this, skel, vec);
    return vec;
}
std::vector<int32_t>  Animation::makeMapping(Skeleton* skel, int root_bone_id) {
    auto bone = skel->getBone(root_bone_id);

    std::vector<int32_t> mapping(nodeCount(), -1);
    std::function<void(Skeleton::Bone&)> walk_bones_fn = [&walk_bones_fn, this, &skel, &mapping](Skeleton::Bone& b){
        int32_t bone_index = b.id;
        int32_t node_index = getNodeIndex(b.name);
        if(node_index >= 0) {
            mapping[node_index] = bone_index;
        }
        
        int32_t child = b.first_child;
        while(child != -1) {
            auto& bc = skel->getBone(child);
            walk_bones_fn(bc);
            child = bc.next_sibling;
        }
    };
    walk_bones_fn(bone);
    return mapping;
}
std::vector<int32_t>  Animation::makeMapping(Skeleton* skel, const char* root_bone_name) {
    auto bone = skel->getBone(root_bone_name);
    if(!bone) {
        assert(false);
        return std::vector<int32_t>(nodeCount(), -1);
    }
    return makeMapping(skel, bone->id);
}


void Animation::sample_one(int node_id, float cursor, AnimSample& sample) {
    sample.t = nodes[node_id].t.at(cursor);
    sample.r = nodes[node_id].r.at(cursor);
    sample.s = nodes[node_id].s.at(cursor);
}

static gfxm::mat4 getParentTransform(Skeleton* skeleton, const std::vector<AnimSample>& samples, int32_t bone_idx) {
    auto bone = skeleton->getBone(bone_idx);
    std::vector<int32_t> chain;
    while(bone.parent >= 0) {
        chain.push_back(bone.parent);
        bone = skeleton->getBone(bone.parent);
    }

    gfxm::mat4 m(1.0f);
    for(int i = chain.size() - 1; i >= 0; --i) {
        gfxm::mat4 lcl = 
            gfxm::translate(gfxm::mat4(1.0f), samples[chain[i]].t)
            * gfxm::to_mat4(samples[chain[i]].r)
            * gfxm::scale(gfxm::mat4(1.0f), samples[chain[i]].s);
        m = lcl * m;
    }
    return m;
}

void Animation::sample_remapped(
    AnimSampleBuffer& sample_buffer,
    float prev_cursor,
    float cursor,
    Skeleton* skeleton,
    const std::vector<int32_t>& mapping
) {
    sample_remapped(sample_buffer.getSamples(), cursor, mapping);

    if(root_motion_node_index >= 0) {
        if(enable_root_motion_t_xz) {
            int32_t rm_bone_id = mapping[root_motion_node_index];

            AnimSampleBuffer zero_pose(skeleton);
            AnimSampleBuffer last_pose(skeleton);
            sample_remapped(zero_pose.getSamples(), .0f, mapping);
            sample_remapped(last_pose.getSamples(), length, mapping);
            gfxm::mat4 m_rm_zero_parent = ::getParentTransform(skeleton, zero_pose.getSamples(), rm_bone_id);
            gfxm::mat4 m_rm_last_parent = ::getParentTransform(skeleton, last_pose.getSamples(), rm_bone_id);
            gfxm::vec3 rm_zero_world_t = m_rm_zero_parent * gfxm::vec4(zero_pose[rm_bone_id].t, 1.0f);
            gfxm::vec3 rm_last_world_t = m_rm_last_parent * gfxm::vec4(last_pose[rm_bone_id].t, 1.0f);

            // Get prev pose
            AnimSampleBuffer prev_pose(skeleton);
            sample_remapped(prev_pose.getSamples(), prev_cursor, mapping);
            gfxm::mat4 m_rm_prev_parent = ::getParentTransform(skeleton, prev_pose.getSamples(), rm_bone_id);
            gfxm::vec3 rm_prev_world_t = m_rm_prev_parent * gfxm::vec4(prev_pose[rm_bone_id].t, 1.0f);

            //
            int root_of_rm_src_index = skeleton->getRootOf(rm_bone_id);
            
            gfxm::mat4 m_rm_parent = ::getParentTransform(skeleton, sample_buffer.getSamples(), rm_bone_id);
            gfxm::vec3 rm_world_t = m_rm_parent * gfxm::vec4(sample_buffer[rm_bone_id].t, 1.0f);

            auto& t = sample_buffer[root_of_rm_src_index].t;
            t.x -= rm_world_t.x;
            t.z -= rm_world_t.z;

            if(prev_cursor <= cursor) {
                sample_buffer.getRootMotionDelta().t = (rm_world_t - rm_prev_world_t) * skeleton->scale_factor;
            } else {
                sample_buffer.getRootMotionDelta().t =
                    ((rm_last_world_t - rm_prev_world_t) + (rm_world_t - rm_zero_world_t)) * skeleton->scale_factor;
            }
        }
    }
}

void Animation::sample_remapped(
    std::vector<AnimSample>& out,
    float cursor,
    const std::map<size_t, size_t>& remap
) {
    for(size_t i = 0; i < nodes.size() && i < out.size(); ++i) {
        auto& n = nodes[i];
        size_t out_index = remap.at(i);
        AnimSample& result = out[out_index];

        result.t = n.t.at(cursor);
        result.r = n.r.at(cursor);
        result.s = n.s.at(cursor);
    }
}
void Animation::sample_remapped(
    std::vector<AnimSample>& samples,
    float cursor,
    const std::vector<int32_t>& remap
) {
    for(size_t i = 0; i < nodes.size() && i < samples.size(); ++i) {
        auto& n = nodes[i];
        int32_t out_index = remap[i];
        if(out_index < 0 || out_index >= samples.size()) continue;

        AnimSample& result = samples[out_index];

        result.t = n.t.at(cursor);
        result.r = n.r.at(cursor);
        result.s = n.s.at(cursor);
    }
}

void Animation::blend_remapped(
    std::vector<AnimSample>& in_out,
    float cursor, float weight,
    const std::map<size_t, size_t>& remap
) {
    for(size_t i = 0; i < nodes.size() && i < in_out.size(); ++i) {
        auto& n = nodes[i];
        size_t out_index = remap.at(i);
        AnimSample& result = in_out[out_index];

        result.t = gfxm::lerp(result.t, n.t.at(cursor), weight);
        result.r = gfxm::slerp(result.r, n.r.at(cursor), weight);
        result.s = gfxm::lerp(result.s, n.s.at(cursor), weight);
    }
}
void Animation::blend_remapped(
    std::vector<AnimSample>& samples,
    float cursor, float weight,
    const std::vector<int32_t>& remap
) {
    for(size_t i = 0; i < nodes.size() && i < samples.size(); ++i) {
        auto& n = nodes[i];
        int32_t out_index = remap[i];
        if(out_index < 0 || out_index >= samples.size()) continue;

        AnimSample& result = samples[out_index];

        result.t = gfxm::lerp(result.t, n.t.at(cursor), weight);
        result.r = gfxm::slerp(result.r, n.r.at(cursor), weight);
        result.s = gfxm::lerp(result.s, n.s.at(cursor), weight);
    }
}

void Animation::additive_blend_remapped(
    std::vector<AnimSample>& in_out,
    float cursor, float weight,
    const std::map<size_t, size_t>& remap
) {
    for(size_t i = 0; i < nodes.size() && i < in_out.size(); ++i) {
        auto& n = nodes[i];
        size_t out_index = remap.at(i);
        AnimSample& result = in_out[out_index];

        gfxm::vec3 t_base = n.t.at(0.0f);
        gfxm::quat r_base = n.r.at(0.0f);
        gfxm::vec3 s_base = n.s.at(0.0f);
        gfxm::vec3 t_cur = n.t.at(cursor);
        gfxm::quat r_cur = n.r.at(cursor);
        gfxm::vec3 s_cur = n.s.at(cursor);

        gfxm::vec3 t = t_cur - t_base;
        gfxm::quat r = r_cur * gfxm::inverse(r_base);
        gfxm::vec3 s = s_cur - s_base;

        result.t += t * weight;
        result.r = gfxm::slerp(gfxm::quat(), r, weight) * result.r;
        result.s += s * weight;
    }
}
void Animation::additive_blend_remapped(
    std::vector<AnimSample>& samples,
    float cursor, float weight,
    const std::vector<int32_t>& remap
) {
    for(size_t i = 0; i < nodes.size() && i < samples.size(); ++i) {
        auto& n = nodes[i];
        int32_t out_index = remap[i];
        if(out_index < 0 || out_index >= samples.size()) continue;

        AnimSample& result = samples[out_index];

        gfxm::vec3 t_base = n.t.at(0.0f);
        gfxm::quat r_base = n.r.at(0.0f);
        gfxm::vec3 s_base = n.s.at(0.0f);
        gfxm::vec3 t_cur = n.t.at(cursor);
        gfxm::quat r_cur = n.r.at(cursor);
        gfxm::vec3 s_cur = n.s.at(cursor);

        gfxm::vec3 t = t_cur - t_base;
        gfxm::quat r = r_cur * gfxm::inverse(r_base);
        gfxm::vec3 s = s_cur - s_base;

        result.t += t * weight;
        result.r = gfxm::slerp(gfxm::quat(), r, weight) * result.r;
        result.s += s * weight;
    }
}

void Animation::serialize(out_stream& out_) {
    ZipWriter zipw;
    {
        std::stringstream out;    

        out.write((char*)&length, sizeof(length));
        out.write((char*)&fps, sizeof(fps));

        uint32_t node_count = nodes.size();
        out.write((char*)&node_count, sizeof(node_count));

        auto write_node = [](std::ostream& out, AnimNode& n) {
            auto& t_kfs = n.t.get_keyframes();
            uint64_t t_count = t_kfs.size();
            out.write((char*)&t_count, sizeof(t_count));
            out.write((char*)t_kfs.data(), t_kfs.size() * sizeof(keyframe<gfxm::vec3>));
            auto& r_kfs = n.r.get_keyframes();
            uint64_t r_count = r_kfs.size();
            out.write((char*)&r_count, sizeof(r_count));
            out.write((char*)r_kfs.data(), r_kfs.size() * sizeof(keyframe<gfxm::quat>));
            auto& s_kfs = n.s.get_keyframes();
            uint64_t s_count = s_kfs.size();
            out.write((char*)&s_count, sizeof(s_count));
            out.write((char*)s_kfs.data(), s_kfs.size() * sizeof(keyframe<gfxm::vec3>));
        };

        for(auto& n : nodes) {
            write_node(out, n);
        }
        write_node(out, root_motion_node);

        for(auto& kv : node_indices) {
            const std::string& name = kv.first;
            size_t id = kv.second;

            uint32_t name_sz = name.size();
            out.write((char*)&name_sz, sizeof(name_sz));
            out.write((char*)name.data(), name_sz);
            uint64_t node_id = (uint64_t)id;
            out.write((char*)&node_id, sizeof(node_id));
        }

        out.write((char*)&root_motion_enabled, sizeof(root_motion_enabled));

        uint32_t rm_name_sz = root_motion_node_name.size();
        out.write((char*)&rm_name_sz, sizeof(rm_name_sz));
        out.write((char*)root_motion_node_name.data(), root_motion_node_name.size());

        out.seekg(0, std::ios::beg);
        zipw.add("nodes", out);
    }
    {
        dstream out;
        DataWriter w(&out);
        w.write<uint32_t>(events.size());
        for(auto& kv : events) {
            w.write(kv.first);
            w.write(kv.second.time);
            w.write(kv.second.threshold);
        }
        zipw.add("events", out.getBuffer());
    }
    {
        dstream out;
        DataWriter w(&out);
        w.write<uint32_t>(extra_curves.size());
        for(auto& kv : extra_curves) {
            w.write(kv.first);
            auto& curve = kv.second;
            w.write(curve.get_keyframes());
        }
        zipw.add("ex_curves", out.getBuffer());
    }
    {
        dstream out;
        DataWriter w(&out);
        uint8_t flags = 0;
        std::string root_motion_node_name = "";

        flags |= enable_root_motion_t_xz ? 1 : 0;
        flags |= enable_root_motion_t_y ? (1 << 1) : 0;
        flags |= enable_root_motion_r_y ? (1 << 2) : 0;
        root_motion_node_name = this->root_motion_node_name;

        w.write<uint8_t>(flags);
        w.write(root_motion_node_name);
        zipw.add("root_motion", out.getBuffer());
    }

    auto buf = zipw.finalize();
    out_.write((char*)buf.data(), buf.size());
}
bool Animation::deserialize(in_stream& in_, size_t sz) { 
    clearNodes();

    std::vector<char> inbuf(sz);
    in_.read(inbuf.data(), sz);
    
    ZipReader zipr(inbuf);
    auto buf = zipr.extractFile("nodes");
    if(!buf.empty()) {
        std::stringstream in;
        in.write((char*)buf.data(), buf.size());
        in.seekg(0, std::ios::beg);

        in.read((char*)&length, sizeof(length));
        in.read((char*)&fps, sizeof(fps));

        uint32_t node_count = 0;
        in.read((char*)&node_count, sizeof(node_count));

        auto read_node = [](std::istream& in, AnimNode& n) {
            uint64_t t_count = 0;
            uint64_t r_count = 0;
            uint64_t s_count = 0;
            std::vector<keyframe<gfxm::vec3>> t_kfs;
            std::vector<keyframe<gfxm::quat>> r_kfs;
            std::vector<keyframe<gfxm::vec3>> s_kfs;

            in.read((char*)&t_count, sizeof(t_count));
            if(t_count) {
                t_kfs.resize(t_count);
                in.read((char*)t_kfs.data(), t_count * sizeof(keyframe<gfxm::vec3>));
                
                n.t.set_keyframes(t_kfs);
            }

            in.read((char*)&r_count, sizeof(r_count));
            if(r_count) {
                r_kfs.resize(r_count);
                in.read((char*)r_kfs.data(), r_count * sizeof(keyframe<gfxm::quat>));
                
                n.r.set_keyframes(r_kfs);
            }
            
            in.read((char*)&s_count, sizeof(s_count));
            if(s_count) {
                s_kfs.resize(s_count);
                in.read((char*)s_kfs.data(), s_count * sizeof(keyframe<gfxm::vec3>));

                n.s.set_keyframes(s_kfs);
            }
        };

        for(uint32_t i = 0; i < node_count; ++i) {
            nodes.emplace_back(AnimNode());
            AnimNode& n = nodes.back();
            read_node(in, n);
        }
        read_node(in, root_motion_node);

        for(uint32_t i = 0; i < node_count; ++i) {
            std::string name;
            uint32_t name_sz = 0;
            in.read((char*)&name_sz, sizeof(name_sz));
            name.resize(name_sz);
            in.read((char*)name.data(), name.size());
            uint64_t node_id = 0;
            in.read((char*)&node_id, sizeof(node_id));

            node_indices[name] = (size_t)node_id;
            node_names.emplace_back(name);
        }

        in.read((char*)&root_motion_enabled, sizeof(root_motion_enabled));

        uint32_t rm_name_sz = 0;
        in.read((char*)&rm_name_sz, sizeof(rm_name_sz));
        root_motion_node_name.resize(rm_name_sz);
        in.read((char*)root_motion_node_name.data(), rm_name_sz);
    }
    buf = zipr.extractFile("events");
    if(!buf.empty()) {
        dstream in;
        in.setBuffer(buf);
        DataReader r(&in);
        uint32_t evt_count = r.read<uint32_t>();
        for(uint32_t i = 0; i < evt_count; ++i) {
            std::string name = r.readStr();
            float t = r.read<float>();
            float thresh = r.read<float>();
            setEvent(name, t, thresh);
        }
    }
    buf = zipr.extractFile("ex_curves");
    if(!buf.empty()) {
        dstream in;
        in.setBuffer(buf);
        DataReader r(&in);
        uint32_t curve_count = r.read<uint32_t>();
        for(uint32_t i = 0; i < curve_count; ++i) {
            std::string name = r.readStr();
            std::vector<keyframe<float>> kfs = r.readArray<keyframe<float>>();
            extra_curves[name].set_keyframes(kfs);
        }
    }
    buf = zipr.extractFile("root_motion");
    if(!buf.empty()) {
        dstream in;
        in.setBuffer(buf);
        DataReader r(&in);
        
        uint8_t flags = r.read<uint8_t>();
        std::string root_motion_node_name = r.readStr();

        if(flags & 1) enable_root_motion_t_xz = true;
        if(flags & 2) enable_root_motion_t_y = true;
        if(flags & 4) enable_root_motion_r_y = true;
        setRootMotionSourceNode(root_motion_node_name);  
    }
    return true;
}