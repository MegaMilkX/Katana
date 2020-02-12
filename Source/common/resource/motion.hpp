#ifndef MOTION_HPP
#define MOTION_HPP

#include "resource.h"
#include "anim_primitive.hpp"

#include "skeleton.hpp"
#include "../scene/game_scene.hpp"

#include <memory>

class Motion : public Resource {
    RTTR_ENABLE(Resource)

    std::unique_ptr<AnimatorBase> animator;
    // TODO: Parameter buffer here

public:
    std::shared_ptr<GameScene> reference;
    std::shared_ptr<Skeleton> skeleton;

    void setSkeleton(std::shared_ptr<Skeleton> skel) {
        if(animator) {
            animator->setSkeleton(skel);
        }
    }

    const char* getWriteExtension() const override { return "motion"; }

    template<typename ANIMATOR_T>
    void resetAnimator() {
        animator.reset(new ANIMATOR_T());
        if(skeleton) {
            animator->setSkeleton(skeleton);
        }
    }
    AnimatorBase* getAnimator() {
        return animator.get();
    }

    void update(float dt, std::vector<AnimSample>& samples) {
        if(animator) {
            animator->update(dt, samples);
        }
    }
};
STATIC_RUN(Motion) {
    rttr::registration::class_<Motion>("Motion")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
