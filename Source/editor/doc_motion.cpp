#include "doc_motion.hpp"


void MotionGuiFSM::drawGui(Editor* ed, float dt) {
    ImGui::Text("FSM");
}
void MotionGuiFSM::drawToolbox(Editor* ed) {
    ImGui::Text("FSM");
}


void MotionGuiBlendTree::drawGui(Editor* ed, float dt) {
    ImGui::Text("BlendTree");
}
void MotionGuiBlendTree::drawToolbox(Editor* ed) {
    ImGui::Text("BlendTree");
}


void DocMotion::resetGui() {
    AnimatorBase* animator = _resource->getAnimator();
    if(!animator) {
        gui.reset();
        return;
    }

    auto type = animator->getType();
    if(type == ANIMATOR_FSM) {
        AnimFSM* fsm = (AnimFSM*)animator; // TODO:

        gui.reset(new MotionGuiFSM(fsm));
    } else if(type == ANIMATOR_BLEND_TREE) {
        BlendTree* bt = (BlendTree*)animator; // TODO:

        gui.reset(new MotionGuiBlendTree(bt));
    }    
}


void DocMotion::onGui(Editor* ed, float dt) {
    if(!gui) {
        ImGui::Text("Create new");
        if(ImGui::Button("Animation FSM")) {
            _resource->resetAnimator<AnimFSM>();
            resetGui();
        }
        ImGui::Text("OR");
        if(ImGui::Button("BlendTree")) {
            _resource->resetAnimator<BlendTree>();
            resetGui();
        }
    } else {
        gui->drawGui(ed, dt);
    }
}

void DocMotion::onGuiToolbox(Editor* ed) {
    if(!gui) {

    } else {
        gui->drawToolbox(ed);
    }
}