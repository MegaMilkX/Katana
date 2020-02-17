#include "motion.hpp"

#include "anim_fsm.hpp"
#include "blend_tree.hpp"


void Motion::serialize(out_stream& out) {
    DataWriter dw(&out);
    if(!animator) {
        dw.write<int8_t>(ANIMATOR_UNKNOWN);
        return;
    }
    
    ANIMATOR_TYPE animator_type = animator->getType();
    dw.write<int8_t>(animator_type);

    animator->write(out);
}
bool Motion::deserialize(in_stream& in, size_t sz) {
    DataReader dr(&in);
    
    ANIMATOR_TYPE animator_type = (ANIMATOR_TYPE)dr.read<int8_t>();
    if(animator_type == ANIMATOR_FSM) {
        resetAnimator<AnimFSM>();
    } else if(animator_type == ANIMATOR_BLEND_TREE) {
        resetAnimator<BlendTree>();
    }

    if(animator) {
        animator->read(in);
    }

    return true;
}