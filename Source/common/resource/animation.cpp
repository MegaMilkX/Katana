#include "animation.hpp"

#include "../../common/util/data_stream.hpp"

size_t Animation::nodeCount() {
    return nodes.size();
}

AnimNode& Animation::getNode(const std::string& name) {
    auto it = node_names.find(name);
    if(it == node_names.end()) {
        node_names[name] = nodes.size();
        nodes.emplace_back(AnimNode());
    }
    return nodes[node_names[name]];
}

int32_t Animation::getNodeIndex(const std::string& name) {
    auto it = node_names.find(name);
    if(it == node_names.end()) {
        return -1;
    }
    return node_names[name];
}

AnimNode& Animation::getRootMotionNode() {
    return root_motion_node;
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
    const std::vector<size_t>& remap
) {
    for(size_t i = 0; i < nodes.size() && i < samples.size(); ++i) {
        auto& n = nodes[i];
        size_t out_index = remap[i];
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
    const std::vector<size_t>& remap
) {
    for(size_t i = 0; i < nodes.size() && i < samples.size(); ++i) {
        auto& n = nodes[i];
        size_t out_index = remap[i];
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
    const std::vector<size_t>& remap
) {
    for(size_t i = 0; i < nodes.size() && i < samples.size(); ++i) {
        auto& n = nodes[i];
        size_t out_index = remap[i];
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

void Animation::serialize(std::ostream& out_) {
    ZipWriter zipw;
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

    for(auto& kv : node_names) {
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
    auto buf = zipw.finalize();
    out_.write((char*)buf.data(), buf.size());
}
bool Animation::deserialize(std::istream& in_, size_t sz) { 
    std::vector<char> inbuf(sz);
    in_.read(inbuf.data(), sz);
    
    ZipReader zipr(inbuf);
    auto buf = zipr.extractFile("nodes");
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

        node_names[name] = (size_t)node_id;
    }

    in.read((char*)&root_motion_enabled, sizeof(root_motion_enabled));

    uint32_t rm_name_sz = 0;
    in.read((char*)&rm_name_sz, sizeof(rm_name_sz));
    root_motion_node_name.resize(rm_name_sz);
    in.read((char*)root_motion_node_name.data(), rm_name_sz);
    return true;
}