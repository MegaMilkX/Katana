#ifndef ANIM_PRIMITIVE_HPP
#define ANIM_PRIMITIVE_HPP

#include "skeleton.hpp"
#include "animation.hpp"

enum ANIMATOR_TYPE {
    ANIMATOR_UNKNOWN,
    ANIMATOR_FSM,
    ANIMATOR_BLEND_TREE
};

class Motion;
class AnimatorBase {
    Motion* motion = 0;

public:
    virtual ~AnimatorBase() {}

    virtual ANIMATOR_TYPE getType() const = 0;

    virtual Motion* getMotion() = 0;

    virtual void rebuild() = 0;
    virtual void update(float dt, AnimSampleBuffer& sample_buffer) = 0;
    virtual AnimSample getRootMotion() = 0;

    virtual void write(out_stream& out) = 0;
    virtual void read(in_stream& in) = 0;
};

#endif
