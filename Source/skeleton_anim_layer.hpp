#ifndef SKELETON_ANIM_LAYER_HPP
#define SKELETON_ANIM_LAYER_HPP

#include "resource/animation.hpp"

class Animator;
class SkeletonAnimLayer {
public:
    enum BlendMode { BASE, BLEND, ADD, BLEND_MODE_LAST };
    static std::string blendModeString(BlendMode m) {
        std::string r = "UNKNOWN";
        switch(m) {
            case BASE: r = "BASE"; break;
            case BLEND: r = "BLEND"; break;
            case ADD: r = "ADD"; break;
        }
        return r;
    }

    int anim_index = 0;
    BlendMode mode = BASE;
    float cursor = 0.0f;
    float speed = 1.0f;
    float weight = 0.0f;

    SkeletonAnimLayer() {}

    void update(
        Animator* animator,
        float dt,
        Skeleton* skeleton,
        std::vector<AnimSample>& t_fin,
        gfxm::vec3& rm_pos_final,
        gfxm::quat& rm_rot_final
    );
};

#endif
