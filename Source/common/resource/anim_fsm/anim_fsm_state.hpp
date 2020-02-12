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

class AnimFSMState {
protected:
    std::string name = "Action";
    gfxm::vec2 editor_pos;
    std::set<AnimFSMTransition*> out_transitions;

    float cursor = .0f; // normalized
    std::shared_ptr<Skeleton> skeleton;

public:
    AnimFSMState();
    virtual ~AnimFSMState();

    virtual rttr::type getType() const {
        return rttr::type::get<AnimFSMState>();
    }
    virtual void onGuiToolbox() = 0;

    virtual void setSkeleton(std::shared_ptr<Skeleton> skel) = 0;

    const std::string& getName() const { return name; }
    void               setName(const std::string& value) { name = value; }
    const gfxm::vec2&  getEditorPos() const { return editor_pos; }
    void               setEditorPos(const gfxm::vec2& value) { editor_pos = value; }

    std::set<AnimFSMTransition*>& getTransitions() {
        return out_transitions;
    }

    virtual void       update(float dt, std::vector<AnimSample>& samples, float weight) = 0;

    virtual void       write(out_stream& out) = 0;
    virtual void       read(in_stream& in)    = 0;
};

class AnimFSMStateClip : public AnimFSMState {
    std::shared_ptr<Animation> anim;
public:
    void setSkeleton(std::shared_ptr<Skeleton> skel) override {
        skeleton = skel;
        if(!skel) {
            return;
        }

        //onSkeletonChanged();
    }

    void update(float dt, std::vector<AnimSample>& samples, float weight) override {
        if(!anim || !skeleton) return;
        anim->sample_remapped(samples, cursor * anim->length, anim->getMapping(skeleton.get()));
        cursor += dt * (anim->fps / anim->length);
        if(cursor > 1.0f) {
            cursor -= 1.0f;
        }
    }

    rttr::type getType() const override {
        return rttr::type::get<AnimFSMStateClip>();
    }
    void onGuiToolbox() override {
        imguiResourceTreeCombo("anim", anim, "anm", [](){
            // TODO: Needs remapping?
            // Or send a signal to recompile whole structure
        });
    }

    void write(out_stream& out) override {
        DataWriter w(&out);
        if(anim) {
            w.write(anim->Name());
        } else {
            w.write(std::string());
        }
    }
    void read(in_stream& in) override    {
        DataReader r(&in);
        std::string anim_name = r.readStr();
        anim = retrieve<Animation>(anim_name);
    }
};

class AnimFSMStateFSM : public AnimFSMState {
    std::shared_ptr<AnimFSM> fsm;
public:
    AnimFSMStateFSM();

    AnimFSM* getFSM() { return fsm.get(); }

    void setSkeleton(std::shared_ptr<Skeleton> skel) override;

    void update(float dt, std::vector<AnimSample>& samples, float weight) override;

    rttr::type getType() const override;
    void onGuiToolbox() override;

    void write(out_stream& out) override;
    void read(in_stream& in) override;
};

class AnimFSMStateBlendTree : public AnimFSMState {
    std::shared_ptr<BlendTree> blend_tree;
public:
    AnimFSMStateBlendTree();

    BlendTree* getBlendTree() { return blend_tree.get(); }

    void setSkeleton(std::shared_ptr<Skeleton> skel) override;

    void update(float dt, std::vector<AnimSample>& samples, float weight) override;

    rttr::type getType() const override;
    void onGuiToolbox() override;

    void write(out_stream& out) override;
    void read(in_stream& in) override;
};

#endif
