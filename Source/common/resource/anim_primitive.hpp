#ifndef ANIM_PRIMITIVE_HPP
#define ANIM_PRIMITIVE_HPP

enum ANIMATOR_TYPE {
    ANIMATOR_UNKNOWN,
    ANIMATOR_FSM,
    ANIMATOR_BLEND_TREE
};

class AnimatorBase {
public:
    virtual ~AnimatorBase() {}

    virtual ANIMATOR_TYPE getType() const = 0;
};

#endif
