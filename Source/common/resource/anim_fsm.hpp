#ifndef ACTION_GRAPH_HPP
#define ACTION_GRAPH_HPP

#include "resource.h"
#include "anim_primitive.hpp"
#include "../gfxm.hpp"

#include "../resource/animation.hpp"
#include "../resource/skeleton.hpp"

#include "../util/anim_blackboard.hpp"

#include "anim_fsm/anim_fsm_state.hpp"

#include "../util/make_next_name.hpp"

#include "motion.hpp"

// ==========================


inline const char* condTypeToCStr(AnimFSMTransition::CONDITION cond) {
    const char* cstr = 0;
    switch(cond) {
    case AnimFSMTransition::LARGER: cstr = ">"; break;
    case AnimFSMTransition::LARGER_EQUAL: cstr = ">="; break;
    case AnimFSMTransition::LESS: cstr = "<"; break;
    case AnimFSMTransition::LESS_EQUAL: cstr = "<="; break;
    case AnimFSMTransition::EQUAL: cstr = "=="; break;
    case AnimFSMTransition::NOT_EQUAL: cstr = "!="; break;
    };
    return cstr;
}


class AnimFSM : public AnimatorBase {
    std::vector<AnimFSMTransition*> transitions;
    std::vector<AnimFSMState*> actions;
    size_t entry_action = 0;
    size_t current_action = 0;
    float trans_weight = 1.0f;
    float trans_speed = 0.1f;
    std::vector<AnimSample> trans_samples;
    Motion* owner_motion = 0;

    int cursor_value = -1;
    int play_count_value = -1;
    int play_count = 0;

    void pickEntryAction();

public:
    AnimFSM(Motion* motion)
    : owner_motion(motion) {
        if(motion) {
            cursor_value = motion->getBlackboard().getIndex("normal_cursor");
            if(cursor_value == -1) {
                cursor_value = motion->getBlackboard().allocValue();
                motion->getBlackboard().setName(cursor_value, "normal_cursor");
            }
            play_count_value = motion->getBlackboard().getIndex("play_count");
            if(play_count_value == -1) {
                play_count_value = motion->getBlackboard().allocValue();
                motion->getBlackboard().setName(play_count_value, "play_count");
            }
        }
    }

    ANIMATOR_TYPE getType() const override { return ANIMATOR_FSM; }

    Motion* getMotion() override {
        return owner_motion;
    }

    void rebuild() override {
        for(auto a : actions) {
            a->rebuild();
        }
    }

    template<typename ANIM_FSM_STATE_T>
    AnimFSMState*       createAction(const std::string& name = "Action");
    void                renameAction(AnimFSMState* action, const std::string& name);
    AnimFSMTransition*  createTransition(const std::string& from, const std::string& to);

    AnimFSMState*       findAction(const std::string& name);
    int32_t             findActionId(const std::string& name);

    void                deleteAction(AnimFSMState* action);
    void                deleteTransition(AnimFSMTransition* transition);

    const std::vector<AnimFSMState*>&           getActions() const;
    const std::vector<AnimFSMTransition*>&      getTransitions() const;

    AnimFSMState*                               getAction(size_t i);

    size_t                                      getEntryActionId();
    AnimFSMState*                               getEntryAction();
    void                                        setEntryAction(const std::string& name);

    AnimFSMState*                               getCurrentAction();

    void incrementPlayCount() { play_count++; }

    void update(float dt, AnimSampleBuffer& sample_buffer) override;
    AnimSample getRootMotion() override { return AnimSample(); }

    void write(out_stream& out) override;
    void read(in_stream& in) override;
};

template<typename ANIM_FSM_STATE_T>
AnimFSMState* AnimFSM::createAction(const std::string& name) {
    ANIM_FSM_STATE_T* state = new ANIM_FSM_STATE_T(this);
    std::set<std::string> existing_names;
    for(auto a : actions) {
        existing_names.insert(a->getName());
    }
    state->setName(makeNextName(existing_names, name));

    actions.emplace_back(state);

    pickEntryAction();

    return state;
}

#endif
