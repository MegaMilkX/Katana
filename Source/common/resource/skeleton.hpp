#ifndef SKELETON_HPP
#define SKELETON_HPP

#include <map>
#include <vector>

#include "resource.h"
#include "../gfxm.hpp"

#include "../util/log.hpp"
#include "../util/serialization.hpp"

class Skeleton : public Resource {
public:
    struct Bone {
        int32_t id;
        int32_t parent;
        std::string name;
        gfxm::mat4 bind_pose;
    };

    ~Skeleton() {
    }

    void addBone(const std::string& name) {
        auto it = name_to_bone.find(name);
        if(it == name_to_bone.end()) {
            int32_t bone_id = (int32_t)bones.size();
            bones.emplace_back(
                Bone{ bone_id, -1, name, gfxm::mat4(1.0f) }
            );
            name_to_bone[name] = bone_id;
        }
    }
    void setDefaultPose(const std::string& bone, const gfxm::mat4& pose) {
        Bone* b = getBone(bone);
        if(b) {
            b->bind_pose = pose;
        }
    }
    void setParent(const std::string& bone, const std::string& parent) {
        Bone* b = getBone(bone);
        Bone* p = getBone(parent);
        if(b && p) {
            b->parent = p->id;
        }
    }

    gfxm::mat4 getParentTransform(const std::string& bone) {
        Bone* b = getBone(bone);
        if(!b) return gfxm::mat4(1.0f);

        if(b->parent >= 0) {
            Bone& p = getBone(b->parent);
            return getWorldTransform(p.name);
        } else {
            return gfxm::mat4(1.0f);
        }
    }

    gfxm::mat4 getWorldTransform(const std::string& bone) {
        Bone* b = getBone(bone);
        if(!b) return gfxm::mat4(1.0f);
        
        if(b->parent >= 0) {
            Bone& p = getBone(b->parent);
            return b->bind_pose * p.bind_pose;
        } else {
            return b->bind_pose;
        }
    }

    size_t boneCount() {
        return bones.size();
    }
    Bone& getBone(size_t i) {
        return bones[i];
    }
    Bone* getBone(const std::string& name) {
        auto it = name_to_bone.find(name);
        if(it == name_to_bone.end()) {
            return 0;
        }
        return &bones[it->second];
    }

    virtual void serialize(out_stream& out) {
        uint32_t bone_count = bones.size();
        out.write(bone_count);
        for(uint32_t i = 0; i < bone_count; ++i) {
            int32_t id = bones[i].id;
            int32_t parent_id = bones[i].parent;
            out.write(id);
            out.write(parent_id);
            out.write<uint64_t>(bones[i].name.size());
            out.write(bones[i].name);
            out.write(bones[i].bind_pose);
        }
        for(auto kv : name_to_bone) {
            out.write<uint64_t>(kv.first.size());
            out.write(kv.first);
            out.write<uint32_t>(kv.second); // Bone id
        }
    }
    virtual bool deserialize(std::istream& in, size_t sz) { 
        uint32_t bone_count = read<uint32_t>(in);
        for(uint32_t i = 0; i < bone_count; ++i) {
            int32_t id = read<int32_t>(in);
            int32_t parent_id = read<int32_t>(in);
            std::string name = rd_string(in);
            gfxm::mat4 pose = read<gfxm::mat4>(in);
            bones.emplace_back(
                Bone {
                    id, parent_id, name, pose
                }
            );
        }
        for(uint32_t i = 0; i < bone_count; ++i) {
            std::string name = rd_string(in);
            uint32_t bone_id = read<uint32_t>(in);
            name_to_bone[name] = bone_id;
        }
        
        return true; 
    }
private:
    std::vector<Bone> bones;
    std::map<std::string, size_t> name_to_bone;
};

#endif
