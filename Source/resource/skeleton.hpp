#ifndef SKELETON_HPP
#define SKELETON_HPP

#include <map>
#include <vector>

#include "resource.h"
#include "../gfxm.hpp"

#include "util/log.hpp"

class Skeleton : public Resource {
public:
    struct Bone {
        size_t id;
        size_t parent;
        std::string name;
        gfxm::mat4 bind_pose;
    };

    ~Skeleton() {
    }

    void addBone(const std::string& name, const gfxm::mat4& bind_pose) {
        auto it = name_to_bone.find(name);
        if(it != name_to_bone.end()) {
            //bones[it->second].bind_pose = bind_pose;
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
private:
    std::vector<Bone> bones;
    std::map<std::string, size_t> name_to_bone;
};

#endif
