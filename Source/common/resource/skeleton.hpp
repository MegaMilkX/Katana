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

    void clearBones() {
        bones.clear();
        name_to_bone.clear();
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
        DataWriter w(&out);

        w.write<uint32_t>(bones.size());
        for(uint32_t i = 0; i < bones.size(); ++i) {
            w.write<int32_t>(bones[i].id);
            w.write<int32_t>(bones[i].parent);
            w.write(bones[i].name);
            w.write(bones[i].bind_pose);
        }
        for(auto kv : name_to_bone) {
            w.write(kv.first); // Bone name
            w.write<uint32_t>(kv.second); // Bone id
        }
    }
    virtual bool deserialize(in_stream& in, size_t sz) { 
        clearBones();

        uint32_t bone_count;
        in.read(bone_count);
        for(uint32_t i = 0; i < bone_count; ++i) {
            int32_t id;
            in.read(id);
            int32_t parent_id;
            in.read(parent_id);
            std::string name = in.readStr(in.read<uint64_t>());
            gfxm::mat4 pose = in.read<gfxm::mat4>();
            bones.emplace_back(
                Bone {
                    id, parent_id, name, pose
                }
            );
        }
        for(uint32_t i = 0; i < bone_count; ++i) {
            std::string name = in.readStr(in.read<uint64_t>());
            uint32_t bone_id = in.read<uint32_t>();
            name_to_bone[name] = bone_id;
        }
        
        return true; 
    }
private:
    std::vector<Bone> bones;
    std::map<std::string, size_t> name_to_bone;
};

#endif
