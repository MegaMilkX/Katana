#ifndef MOTION_GUI_FSM_HPP
#define MOTION_GUI_FSM_HPP


#include "motion_gui.hpp"

#include "../../common/scene/game_scene.hpp"
#include "../../common/gui_viewport.hpp"
#include "../../common/attributes/light_source.hpp"

#include "../../common/attributes/skeleton_ref.hpp"

class MotionGuiFSM : public MotionGuiBase {
    AnimFSM* fsm;
    AnimFSMState* selected_action = 0;
    AnimFSMTransition* selected_transition = 0;

    std::vector<AnimSample> sample_buffer;

public:
    MotionGuiFSM(const std::string& title, DocMotion* doc, AnimFSM* fsm);

    void drawGui(Editor* ed, float dt) override;
    void drawToolbox(Editor* ed) override;
};


#endif
