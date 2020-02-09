#include "anim_fsm_state.hpp"

AnimFSMState::AnimFSMState() {
    motion.reset(new ClipMotion());
}

AnimFSMState::~AnimFSMState() {
    
}

void AnimFSMState::update(
    float dt, 
    std::vector<AnimSample>& samples,
    float weight
) {
    auto& pose = motion->getPose();
    motion->advance(dt);

    for(size_t i = 0; i < pose.size() && i < samples.size(); ++i) {
        samples[i] = pose[i];
    }
}