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


class AnimFSM : public Resource, public AnimatorBase {
    RTTR_ENABLE(Resource)

    std::vector<AnimFSMTransition*> transitions;
    std::vector<AnimFSMState*> actions;
    size_t entry_action = 0;
    size_t current_action = 0;
    float trans_weight = 1.0f;
    float trans_speed = 0.1f;
    std::vector<AnimSample> trans_samples;
    AnimBlackboard* blackboard = 0;

    void pickEntryAction();

public:
    // For editor purposes
    std::shared_ptr<GameScene> reference_object;
    std::shared_ptr<Skeleton> reference_skel;

    const char* getWriteExtension() const override { return "action_graph"; }

    ANIMATOR_TYPE getType() const override { return ANIMATOR_FSM; }

    void setSkeleton(std::shared_ptr<Skeleton> skeleton) override {
        for(auto a : actions) {
            a->setSkeleton(skeleton);
        }
    }
    void setBlackboard(AnimBlackboard* bb) {
        blackboard = bb;
        for(auto& t : transitions) {
            for(auto& c : t->conditions) {
                c.param_hdl = blackboard->getHandle(c.param_name.c_str());
            }
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

    const std::vector<AnimFSMState*>&       getActions() const;
    const std::vector<AnimFSMTransition*>& getTransitions() const;

    AnimFSMState*                           getAction(size_t i);

    size_t                                     getEntryActionId();
    AnimFSMState*                           getEntryAction();
    void                                       setEntryAction(const std::string& name);

    void update(float dt, std::vector<AnimSample>& samples) override;

    void serialize(out_stream& out) override;
    bool deserialize(in_stream& in, size_t sz) override;
};
STATIC_RUN(AnimFSM) {
    rttr::registration::class_<AnimFSM>("AnimFSM")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

template<typename ANIM_FSM_STATE_T>
AnimFSMState* AnimFSM::createAction(const std::string& name) {
    ANIM_FSM_STATE_T* state = new ANIM_FSM_STATE_T();
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
