#ifndef SKELETON_HPP
#define SKELETON_HPP

#include <map>
#include <vector>

#include "resource.h"
#include "../gfxm.hpp"

#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>

class Skeleton : public Resource {
public:
    struct Bone {
        size_t id;
        size_t parent;
        std::string name;
        gfxm::mat4 bind_pose;
    };

    ~Skeleton() {
        if(ozz_skel)
            ozz::memory::default_allocator()->Delete(ozz_skel);
    }

    void addBone(const std::string& name, const gfxm::mat4& bind_pose) {
        auto it = name_to_bone.find(name);
        if(it != name_to_bone.end()) {
            bones[it->second].bind_pose = bind_pose;
        } else {
            size_t bone_id = bones.size();
            bones.emplace_back(
                Bone{ bone_id, 0, name, bind_pose }
            );
            name_to_bone[name] = bone_id;
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

    bool buildOzzSkeleton() {
        ozz::animation::offline::RawSkeleton raw_skel;
        raw_skel.roots.resize(bones.size());
        
        int i = 0;
        for(auto b : bones) {
            ozz::animation::offline::RawSkeleton::Joint& j = raw_skel.roots[i];
            j.name = b.name.c_str();
            ++i;
        }

        if(!raw_skel.Validate()) {
            LOG_WARN("Failed to validate skeleton");
            return false;
        }
        ozz::animation::offline::SkeletonBuilder builder;
        ozz_skel = builder(raw_skel);
        return true;
    }
    ozz::animation::Skeleton* getOzzSkeleton() {
        return ozz_skel;
    }

    virtual bool Build(DataSourceRef r) {
        return false;
    }
    virtual bool Serialize(std::vector<unsigned char>& data) {
        return false;
    }
private:
    std::vector<Bone> bones;
    std::map<std::string, size_t> name_to_bone;

    ozz::animation::Skeleton* ozz_skel = 0;
};

#endif
