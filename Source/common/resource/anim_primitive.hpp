#ifndef ANIM_PRIMITIVE_HPP
#define ANIM_PRIMITIVE_HPP

#include "skeleton.hpp"

enum ANIMATOR_TYPE {
    ANIMATOR_UNKNOWN,
    ANIMATOR_FSM,
    ANIMATOR_BLEND_TREE
};

class AnimatorBase {
public:
    virtual ~AnimatorBase() {}

    virtual ANIMATOR_TYPE getType() const = 0;

    virtual void setSkeleton(std::shared_ptr<Skeleton> skel) = 0;
    virtual void update(float dt, std::vector<AnimSample>& samples) = 0;

};

#endif
