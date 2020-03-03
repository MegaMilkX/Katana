#ifndef ANIM_FSM_STATE_HPP
#define ANIM_FSM_STATE_HPP

#include "anim_fsm_transition.hpp"
#include "../../gfxm.hpp"

#include "../skeleton.hpp"
#include "../animation.hpp"
#include "../blend_tree.hpp"

#include "../../util/imgui_helpers.hpp"

#include <memory>
#include <set>

enum ANIM_FSM_STATE_TYPE {
    ANIM_FSM_STATE_UNKNOWN,
    ANIM_FSM_STATE_CLIP,
    ANIM_FSM_STATE_FSM,
    ANIM_FSM_STATE_BLEND_TREE
};

class AnimFSM;

class AnimFSMState {
protected:
    AnimFSM*                        owner_fsm = 0;
    std::string                     name = "Action";
    gfxm::vec2                      editor_pos;
    std::set<AnimFSMTransition*>    out_transitions;

    float                           prev_cursor = .0f;
    float                           cursor = .0f; // normalized

public:
    AnimFSMState(AnimFSM* fsm);
    virtual ~AnimFSMState();

    virtual ANIM_FSM_STATE_TYPE getType() const {
        return ANIM_FSM_STATE_UNKNOWN;
    }
    virtual void onGuiToolbox() = 0;

    const std::string& getName() const { return name; }
    void               setName(const std::string& value) { name = value; }
    const gfxm::vec2&  getEditorPos() const { return editor_pos; }
    void               setEditorPos(const gfxm::vec2& value) { editor_pos = value; }

    std::set<AnimFSMTransition*>& getTransitions() {
        return out_transitions;
    }

    virtual void       rebuild() = 0;

    virtual void       update(float dt, AnimSampleBuffer& sample_buffer, Skeleton* skeleton, float weight) = 0;

    virtual void       write(out_stream& out) = 0;
    virtual void       read(in_stream& in)    = 0;
};

class AnimFSMStateClip : public AnimFSMState {
    std::shared_ptr<Animation> anim;
    std::vector<int32_t> mapping;
public:
    AnimFSMStateClip(AnimFSM* fsm);

    void setAnim(std::shared_ptr<Animation> anim);

    void rebuild() override;

    void update(float dt, AnimSampleBuffer& sample_buffer, Skeleton* skeleton, float weight) override;

    ANIM_FSM_STATE_TYPE getType() const override;
    void onGuiToolbox() override;

    void write(out_stream& out) override;
    void read(in_stream& in) override;
};

class AnimFSMStateFSM : public AnimFSMState {
    std::shared_ptr<AnimFSM> fsm;
public:
    AnimFSMStateFSM(AnimFSM* fsm);

    AnimFSM* getFSM() { return fsm.get(); }

    void rebuild() override;

    void update(float dt, AnimSampleBuffer& sample_buffer, Skeleton* skeleton, float weight) override;

    ANIM_FSM_STATE_TYPE getType() const override;
    void onGuiToolbox() override;

    void write(out_stream& out) override;
    void read(in_stream& in) override;
};

class AnimFSMStateBlendTree : public AnimFSMState {
    std::shared_ptr<BlendTree> blend_tree;
public:
    AnimFSMStateBlendTree(AnimFSM* fsm);

    BlendTree* getBlendTree() { return blend_tree.get(); }

    void rebuild() override;

    void update(float dt, AnimSampleBuffer& sample_buffer, Skeleton* skeleton, float weight) override;

    ANIM_FSM_STATE_TYPE getType() const override;
    void onGuiToolbox() override;

    void write(out_stream& out) override;
    void read(in_stream& in) override;
};

#endif
