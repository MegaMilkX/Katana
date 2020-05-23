#include "motion_gui_fsm.hpp"

#include "fsm_gui_elements.hpp"

#include "../../common/util/imgui_helpers.hpp"

#include "../doc_motion.hpp"



MotionGuiFSM::MotionGuiFSM(const std::string& title, DocMotion* doc, AnimFSM* fsm)
: MotionGuiBase(title, doc), fsm(fsm) {

}

static bool s_creating_transition = false;
void MotionGuiFSM::drawGui(Editor* ed, float dt) {
    if(BeginGridView("test")) {
        for(auto& t : fsm->getTransitions()) {
            gfxm::vec2 from = t->from->getEditorPos();
            gfxm::vec2 to = t->to->getEditorPos();
            if(TransitionLine(ImVec2(from.x, from.y), ImVec2(to.x, to.y), selected_transition == t)) {
                selected_transition = t;
            }
        }

        for(auto& a : fsm->getActions()) {
            gfxm::vec2 ed_pos = a->getEditorPos();
            ImVec2 node_pos(ed_pos.x, ed_pos.y);
            bool dbl_clicked = false;
            int node_flags = 0;
            if(selected_action == a) {
                node_flags |= NODE_FLAG_SELECTED;
            }
            if(a == fsm->getEntryAction()) {
                node_flags |= NODE_FLAG_HIGHLIGHT;
            }
            if(a == fsm->getCurrentAction()) {
                node_flags |= NODE_FLAG_PLAYING;
            }

            if(a->getType() == ANIM_FSM_STATE_CLIP) {
                node_flags |= NODE_FLAG_CLIP;
            } else if(a->getType() == ANIM_FSM_STATE_FSM) {
                node_flags |= NODE_FLAG_FSM;
            } else if(a->getType() == ANIM_FSM_STATE_BLEND_TREE) {
                node_flags |= NODE_FLAG_BLEND_TREE;
            }

            if(Node(a->getName().c_str(), node_pos, ImVec2(200, 50), node_flags, &dbl_clicked)) {
                if(s_creating_transition && (selected_action != a)) {
                    fsm->createTransition(selected_action->getName(), a->getName());
                    s_creating_transition = false;
                }
                selected_action = a;
                if(dbl_clicked) {
                    s_creating_transition = true;
                }
            }
            a->setEditorPos(gfxm::vec2(node_pos.x, node_pos.y));
        }
        if(s_creating_transition && selected_action) {
            ImVec2 node_pos;
            gfxm::vec2 node_pos_gfxm = selected_action->getEditorPos();
            node_pos.x = node_pos_gfxm.x;
            node_pos.y = node_pos_gfxm.y;
            TransitionLine(node_pos, GraphEditGridScreenToPos(ImGui::GetMousePos()));
        }
    }
    EndGridView();
}

void MotionGuiFSM::drawToolbox(Editor* ed) {
    ImGui::Text("Add state");
    if(ImGui::Button("Clip", ImVec2(ImGui::GetContentRegionAvailWidth(), .0f))) {
        auto act = fsm->createAction<AnimFSMStateClip>("Clip state");
        ImVec2 new_pos;// = GraphEditGridScreenToPos(s_graph_edit_bb.Min + s_graph_edit_bb.Max * 0.5f);
        act->setEditorPos(gfxm::vec2(new_pos.x, new_pos.y));
        selected_action = act;
    }
    if(ImGui::Button("FSM", ImVec2(ImGui::GetContentRegionAvailWidth(), .0f))) {
        auto act = fsm->createAction<AnimFSMStateFSM>("FSM state");
        ImVec2 new_pos;// = GraphEditGridScreenToPos(s_graph_edit_bb.Min + s_graph_edit_bb.Max * 0.5f);
        act->setEditorPos(gfxm::vec2(new_pos.x, new_pos.y));
        selected_action = act;
    }
    if(ImGui::Button("BlendTree", ImVec2(ImGui::GetContentRegionAvailWidth(), .0f))) {
        auto act = fsm->createAction<AnimFSMStateBlendTree>("BlendTree state");
        ImVec2 new_pos;// = GraphEditGridScreenToPos(s_graph_edit_bb.Min + s_graph_edit_bb.Max * 0.5f);
        act->setEditorPos(gfxm::vec2(new_pos.x, new_pos.y));
        selected_action = act;
    }
    if(selected_action) {
        ImGuiExt::BeginInsetSegment(ImGui::ColorConvertFloat4ToU32(ImVec4(.3f, .15f, .0f, 1.f)));
        ImGui::Text("Selected state");

        bool is_startup = selected_action == fsm->getEntryAction();
        if(ImGui::Checkbox("Startup", &is_startup)) {
            if(is_startup) {
                fsm->setEntryAction(selected_action->getName());
            } else {
                is_startup = true;
            }
        }

        char buf[256];
        memset(buf, 0, sizeof(buf));
        memcpy(buf, selected_action->getName().c_str(), selected_action->getName().size());
        if(ImGui::InputText("name", buf, sizeof(buf))) {
            fsm->renameAction(selected_action, buf);
        }
        
        selected_action->onGuiToolbox();
        
        if(selected_action->getType() == ANIM_FSM_STATE_FSM) {
            if(ImGui::Button(ICON_MDI_PENCIL " Edit FSM", ImVec2(ImGui::GetWindowContentRegionWidth(), .0f))) {
                //setNestedWindow(new DocActionGraph());
                AnimFSMStateFSM* fsmState = (AnimFSMStateFSM*)selected_action;
                doc->pushGuiLayer(new MotionGuiFSM(MKSTR(ICON_MDI_SETTINGS << " " << selected_action->getName()), doc, fsmState->getFSM()));
            }
        } else if(selected_action->getType() == ANIM_FSM_STATE_BLEND_TREE) {
            if(ImGui::Button(ICON_MDI_PENCIL " Edit BlendTree", ImVec2(ImGui::GetWindowContentRegionWidth(), .0f))) {
                AnimFSMStateBlendTree* state = (AnimFSMStateBlendTree*)selected_action;
                doc->pushGuiLayer(new MotionGuiBlendTree(MKSTR("[BT] " << selected_action->getName()), doc, state->getBlendTree()));
            }
        }

        if(ImGui::Button("Delete action", ImVec2(ImGui::GetWindowContentRegionWidth(), .0f))) {
            fsm->deleteAction(selected_action);
            selected_action = 0;
            selected_transition = 0;
        }

        ImGuiExt::EndInsetSegment();
    }
    ImGui::Separator();
    if(selected_transition) {
        ImGuiExt::BeginInsetSegment(ImGui::ColorConvertFloat4ToU32(ImVec4(.0f, .3f, .15f, 1.f)));
        ImGui::Text("Selected transition");

        ImGui::Text(MKSTR(selected_transition->from->getName() << " -> " << selected_transition->to->getName()).c_str());
        ImGui::SameLine();
        if(ImGui::SmallButton(ICON_MDI_DELETE_EMPTY )) {
            fsm->deleteTransition(selected_transition);
            selected_transition = 0;
        } else {
            ImGui::DragFloat("blend time", &selected_transition->blendTime);
            auto& conds = selected_transition->conditions;
            for(size_t i = 0; i < conds.size(); ++i) {
                auto& cond = conds[i];
                ImGui::PushItemWidth(70);
                if(ImGui::BeginCombo(MKSTR("###cond_id"<<i).c_str(), conds[i].param_name.c_str(), ImGuiComboFlags_NoArrowButton)) {
                    for(auto it = fsm->getMotion()->getBlackboard().begin(); it != false; ++it) {
                        if(ImGui::Selectable((*it).name.c_str(), cond.param_hdl == it.getIndex())) {
                            cond.param_hdl = it.getIndex();
                            cond.param_name = (*it).name;
                        }
                    }
                    ImGui::EndCombo();
                } 
                ImGui::PopItemWidth();
                ImGui::SameLine();
                ImGui::PushItemWidth(30);
                if(ImGui::BeginCombo(MKSTR("###cond_type"<<i).c_str(), condTypeToCStr(cond.type), ImGuiComboFlags_NoArrowButton)) {
                    if(ImGui::Selectable(">")) { cond.type = AnimFSMTransition::LARGER; }
                    if(ImGui::Selectable(">=")) { cond.type = AnimFSMTransition::LARGER_EQUAL; }
                    if(ImGui::Selectable("<")) { cond.type = AnimFSMTransition::LESS; }
                    if(ImGui::Selectable("<=")) { cond.type = AnimFSMTransition::LESS_EQUAL; }
                    if(ImGui::Selectable("==")) { cond.type = AnimFSMTransition::EQUAL; }
                    if(ImGui::Selectable("!=")) { cond.type = AnimFSMTransition::NOT_EQUAL; }
                    ImGui::EndCombo();
                }
                ImGui::PopItemWidth();
                //ImGui::Text(">=");
                ImGui::SameLine();
                ImGui::PushItemWidth(60);
                ImGui::DragFloat(MKSTR("###cond_value" << i).c_str(), &cond.ref_value, .01f);
                ImGui::PopItemWidth();
            }
            if(ImGui::SmallButton(ICON_MDI_PLUS "###cond_add")) {
                selected_transition->conditions.emplace_back(
                    AnimFSMTransition::Condition{
                        -1, "Param", AnimFSMTransition::CONDITION::LARGER, .0f
                    }
                );
            }
        }

        ImGuiExt::EndInsetSegment();
    }
}
