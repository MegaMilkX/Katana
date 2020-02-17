#include "anim_fsm_state.hpp"

#include "../../common/resource/anim_fsm.hpp"


AnimFSMState::AnimFSMState(AnimFSM* fsm)
: owner_fsm(fsm) {

}

AnimFSMState::~AnimFSMState() {

}

// === AnimFSMStateClip ============


AnimFSMStateClip::AnimFSMStateClip(AnimFSM* fsm)
: AnimFSMState(fsm) {}

void AnimFSMStateClip::setAnim(std::shared_ptr<Animation> anim) {
    this->anim = anim;
    rebuild();
}

void AnimFSMStateClip::rebuild() {
    if(!anim || !owner_fsm->getMotion()->getSkeleton()) {
        mapping.clear();
        return;
    }
    mapping = anim->getMapping(owner_fsm->getMotion()->getSkeleton().get());
}

void AnimFSMStateClip::update(float dt, std::vector<AnimSample>& samples, float weight) {
    if(!anim || mapping.empty()) return;
    anim->sample_remapped(samples, cursor * anim->length, mapping);
    cursor += dt * (anim->fps / anim->length);
    if(cursor > 1.0f) {
        cursor -= 1.0f;
    }
}

ANIM_FSM_STATE_TYPE AnimFSMStateClip::getType() const {
    return ANIM_FSM_STATE_CLIP;
}
void AnimFSMStateClip::onGuiToolbox() {
    imguiResourceTreeCombo("anim", anim, "anm", [this](){
        setAnim(anim);
    });
}

void AnimFSMStateClip::write(out_stream& out) {
    DataWriter w(&out);
    if(anim) {
        w.write(anim->Name());
    } else {
        w.write(std::string());
    }
}
void AnimFSMStateClip::read(in_stream& in) {
    DataReader r(&in);
    std::string anim_name = r.readStr();
    anim = retrieve<Animation>(anim_name);
}


// === AnimFSMStateFSM =============

AnimFSMStateFSM::AnimFSMStateFSM(AnimFSM* owner_fsm)
: AnimFSMState(owner_fsm) {
    fsm.reset(new AnimFSM(owner_fsm->getMotion()));
}

void AnimFSMStateFSM::rebuild() {
    fsm->rebuild();
}

void AnimFSMStateFSM::update(float dt, std::vector<AnimSample>& samples, float weight) {
    fsm->update(dt, samples);
}

ANIM_FSM_STATE_TYPE AnimFSMStateFSM::getType() const {
    return ANIM_FSM_STATE_FSM;
}
void AnimFSMStateFSM::onGuiToolbox() {
    
}

void AnimFSMStateFSM::write(out_stream& out) {
    fsm->write(out);
}
void AnimFSMStateFSM::read(in_stream& in)    {
    fsm->read(in);
}

// === AnimFSMStateBlendTree =============

#include "../../common/resource/blend_tree.hpp"

AnimFSMStateBlendTree::AnimFSMStateBlendTree(AnimFSM* owner_fsm) 
: AnimFSMState(owner_fsm) {
    blend_tree.reset(new BlendTree(owner_fsm->getMotion()));
}

void AnimFSMStateBlendTree::rebuild() {
    blend_tree->rebuild();
}

void AnimFSMStateBlendTree::update(float dt, std::vector<AnimSample>& samples, float weight) {
    blend_tree->update(dt, samples);
}
ANIM_FSM_STATE_TYPE AnimFSMStateBlendTree::getType() const {
    return ANIM_FSM_STATE_BLEND_TREE;
}
void AnimFSMStateBlendTree::onGuiToolbox() {
    
}

void AnimFSMStateBlendTree::write(out_stream& out) {
    blend_tree->write(out);
}
void AnimFSMStateBlendTree::read(in_stream& in) {
    blend_tree->read(in);
}