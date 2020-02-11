#ifndef MOTION_HPP
#define MOTION_HPP

#include "resource.h"
#include "anim_primitive.hpp"

#include "skeleton.hpp"

#include <memory>

class Motion : public Resource {
    RTTR_ENABLE(Resource)

    std::string reference_scene;
    std::shared_ptr<Skeleton> skeleton;

    std::unique_ptr<AnimatorBase> animator;
    // TODO: Parameter buffer here

public:
    const char* getWriteExtension() const override { return "motion"; }

    template<typename ANIMATOR_T>
    void resetAnimator() {
        animator.reset(new ANIMATOR_T());
    }
    AnimatorBase* getAnimator() {
        return animator.get();
    }
};
STATIC_RUN(Motion) {
    rttr::registration::class_<Motion>("Motion")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
