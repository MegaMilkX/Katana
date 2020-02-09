#ifndef ANIM_FSM_STATE_HPP
#define ANIM_FSM_STATE_HPP

#include "anim_fsm_transition.hpp"
#include "../../gfxm.hpp"

#include "../clip_motion.hpp"
#include "../blend_tree_motion.hpp"

#include "../skeleton.hpp"

#include <memory>
#include <set>

class AnimFSMState {
    std::string name = "Action";
    gfxm::vec2 editor_pos;
    std::set<AnimFSMTransition*> out_transitions;
public:
    std::shared_ptr<Motion> motion;

    AnimFSMState();

    void setSkeleton(std::shared_ptr<Skeleton> skel) {
        motion->setSkeleton(skel);
    }

    const std::string& getName() const { return name; }
    void               setName(const std::string& value) { name = value; }
    const gfxm::vec2&  getEditorPos() const { return editor_pos; }
    void               setEditorPos(const gfxm::vec2& value) { editor_pos = value; }

    std::set<AnimFSMTransition*>& getTransitions() {
        return out_transitions;
    }

    void               update(
        float dt, 
        std::vector<AnimSample>& samples,
        float weight
    );

    void               write(out_stream& out);
    void               read(in_stream& in);
};

#endif
