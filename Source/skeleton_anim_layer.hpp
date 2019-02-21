#ifndef SKELETON_ANIM_LAYER_HPP
#define SKELETON_ANIM_LAYER_HPP

#include "resource/animation.hpp"

enum ANIM_BLEND_MODE {
    ANIM_MODE_NONE,
    ANIM_MODE_BLEND,
    ANIM_MODE_ADD,
    ANIM_MODE_LAST
};

class Animator;
class SkeletonAnimLayer {
public:
    static std::string blendModeString(ANIM_BLEND_MODE m) {
        std::string r = "UNKNOWN";
        switch(m) {
            case ANIM_MODE_NONE: r = "BASE"; break;
            case ANIM_MODE_BLEND: r = "BLEND"; break;
            case ANIM_MODE_ADD: r = "ADD"; break;
        }
        return r;
    }

    int             anim_index = 0;
    ANIM_BLEND_MODE mode = ANIM_MODE_NONE;
    float           cursor = 0.0f;
    float           speed = 1.0f;
    float           weight = 0.0f;

    bool stopped = false;

    SkeletonAnimLayer() {}

    void update(
        Animator* animator,
        float dt,
        Skeleton* skeleton,
        std::vector<AnimSample>& t_fin,
        gfxm::vec3& rm_pos_final,
        gfxm::quat& rm_rot_final
    );

    int blend_target_index = 0;
    float blend_target_cursor = 0;
    float blend_target_prev_cursor = 0;
    float blend_over_weight = 0;
    float blend_over_speed = 0;
};

#endif
