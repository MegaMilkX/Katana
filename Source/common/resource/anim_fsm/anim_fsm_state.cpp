#include "anim_fsm_state.hpp"

AnimFSMState::AnimFSMState() {

}

AnimFSMState::~AnimFSMState() {

}


// === AnimFSMStateFSM =============

#include "../../common/resource/anim_fsm.hpp"

AnimFSMStateFSM::AnimFSMStateFSM() {
    fsm.reset(new AnimFSM());
}

void AnimFSMStateFSM::setSkeleton(std::shared_ptr<Skeleton> skel) {
    fsm->setSkeleton(skel);
}

void AnimFSMStateFSM::update(float dt, std::vector<AnimSample>& samples, float weight) {
    fsm->update(dt, samples);
}

rttr::type AnimFSMStateFSM::getType() const {
    return rttr::type::get<AnimFSMStateFSM>();
}
void AnimFSMStateFSM::onGuiToolbox() {
    
}

void AnimFSMStateFSM::write(out_stream& out) {}
void AnimFSMStateFSM::read(in_stream& in)    {}

// === AnimFSMStateBlendTree =============

#include "../../common/resource/blend_tree.hpp"

AnimFSMStateBlendTree::AnimFSMStateBlendTree() {
    blend_tree.reset(new BlendTree());
}

void AnimFSMStateBlendTree::setSkeleton(std::shared_ptr<Skeleton> skel) {
    blend_tree->setSkeleton(skel);
}

void AnimFSMStateBlendTree::update(float dt, std::vector<AnimSample>& samples, float weight) {
    blend_tree->update(dt, samples);
}
rttr::type AnimFSMStateBlendTree::getType() const {
    return rttr::type::get<AnimFSMStateBlendTree>();
}
void AnimFSMStateBlendTree::onGuiToolbox() {
    
}

void AnimFSMStateBlendTree::write(out_stream& out) {}
void AnimFSMStateBlendTree::read(in_stream& in) {}