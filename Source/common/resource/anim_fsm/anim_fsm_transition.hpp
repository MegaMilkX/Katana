#ifndef ANIM_FSM_TRANSITION_HPP
#define ANIM_FSM_TRANSITION_HPP

#include <string>
#include <vector>

class AnimFSM;

class AnimFSMState;
struct AnimFSMTransition {
    enum CONDITION {
        LARGER,
        LARGER_EQUAL,
        LESS,
        LESS_EQUAL,
        EQUAL,
        NOT_EQUAL
    };
    struct Condition {
        int param_hdl;
        std::string param_name;
        CONDITION type;
        float ref_value;
    };

    float blendTime = 0.1f;
    AnimFSMState* from = 0;
    AnimFSMState* to = 0;
    std::vector<Condition> conditions;
};

#endif
