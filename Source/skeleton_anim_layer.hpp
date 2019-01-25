#ifndef SKELETON_ANIM_LAYER_HPP
#define SKELETON_ANIM_LAYER_HPP

#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/blending_job.h>

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
        ozz::animation::Skeleton* skeleton,
        ozz::Range<ozz::math::SoaTransform>& locals_fin,
        ozz::animation::SamplingCache* cache,
        gfxm::vec3& rm_pos_final,
        gfxm::quat& rm_rot_final
    );
private:
    ozz::Range<ozz::math::SoaTransform> locals;
};

#endif
