#ifndef ANIMATION_HPP
#define ANIMATION_HPP

#include "resource.h"
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/offline/raw_animation.h>
#include <ozz/animation/offline/animation_builder.h>
#include <ozz/animation/offline/additive_animation_builder.h>

#include "../util/animation/curve.h"

#include "skeleton.hpp"

class Animation : public Resource {
public:
    ozz::animation::Animation* anim = 0;
    std::string root_motion_node;
    bool root_motion_enabled = false;
    curve<gfxm::vec3> root_motion_pos;
    curve<gfxm::quat> root_motion_rot;

    void initTracks(std::shared_ptr<Skeleton> skel);

    ~Animation() {
        if(anim)
            ozz::memory::default_allocator()->Delete(anim);
    }
};

#endif
