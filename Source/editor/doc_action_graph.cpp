#include "doc_action_graph.hpp"

#include "../common/util/imgui_helpers.hpp"

#include "motion_gui/fsm_gui_elements.hpp"

DocActionGraph::DocActionGraph() {
    viewport.camMode(GuiViewport::CAM_ORBIT);
    viewport.enableDebugDraw(false);

    onResourceSet();
}

bool s_creating_transition = false;
void DocActionGraph::onGui(Editor* ed, float dt) {
    auto& action_graph = _resource;

    ImGui::BeginColumns("First", 2);

    if(BeginGridView("test")) {
        for(auto& t : action_graph->getTransitions()) {
            gfxm::vec2 from = t->from->getEditorPos();
            gfxm::vec2 to = t->to->getEditorPos();
            if(TransitionLine(ImVec2(from.x, from.y), ImVec2(to.x, to.y), selected_transition == t)) {
                selected_transition = t;
            }
        }

        for(auto& a : action_graph->getActions()) {
            gfxm::vec2 ed_pos = a->getEditorPos();
            ImVec2 node_pos(ed_pos.x, ed_pos.y);
            bool dbl_clicked = false;
            int node_flags = 0;
            if(selected_action == a) {
                node_flags |= NODE_FLAG_SELECTED;
            }
            if(a == action_graph->getEntryAction()) {
                node_flags |= NODE_FLAG_HIGHLIGHT;
            }

            if(a->getType() == rttr::type::get<AnimFSMStateClip>()) {
                node_flags |= NODE_FLAG_CLIP;
            } else if(a->getType() == rttr::type::get<AnimFSMStateFSM>()) {
                node_flags |= NODE_FLAG_FSM;
            } else if(a->getType() == rttr::type::get<AnimFSMStateBlendTree>()) {
                node_flags |= NODE_FLAG_BLEND_TREE;
            }

            if(Node(a->getName().c_str(), node_pos, ImVec2(200, 50), node_flags, &dbl_clicked)) {
                if(s_creating_transition && (selected_action != a)) {
                    action_graph->createTransition(selected_action->getName(), a->getName());
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

    ImGui::NextColumn();  // ================================

    std::vector<ktNode*> tgt_nodes;
    if(action_graph->reference_skel) {
        auto& skel = action_graph->reference_skel;

        tgt_nodes.resize(skel->boneCount());

        action_graph->update(dt, sample_buffer);

        for(size_t i = 0; i < skel->boneCount(); ++i) {
            auto& bone = skel->getBone(i);
            ktNode* node = scn.findObject(bone.name);
            tgt_nodes[i] = node;
        }
        for(size_t i = 0; i < sample_buffer.size(); ++i) {
            auto n = tgt_nodes[i];
            if(n) {
                n->getTransform()->setPosition(sample_buffer[i].t);
                n->getTransform()->setRotation(sample_buffer[i].r);
                n->getTransform()->setScale(sample_buffer[i].s);
            }
        }
    }


    if(cam_pivot) {
        viewport.camSetPivot(cam_pivot->getTransform()->getWorldPosition());
    }
    if(cam_light) {
        cam_light->getOwner()->getTransform()->setTransform(gfxm::inverse(viewport.getView()));
    }
    auto skel_ref = scn.find<SkeletonRef>();
    if(skel_ref) {
        //skel_ref->debugDraw(&viewport.getDebugDraw());
    }

    viewport.draw(&scn);

    ImGui::EndColumns();
}

void DocActionGraph::setReferenceObject(ktNode* node) {
    scn.clear();
                    
    scn.copy(node);
    scn.getTransform()->setScale(
        node->getTransform()->getScale()
    );
    gfxm::aabb box;
    scn.makeAabb(box);

    cam_light = scn.createChild()->get<DirLight>().get();
    cam_light->intensity = 500.0f;

    viewport.resetCamera((box.from + box.to) * 0.5f, gfxm::length(box.to - box.from));

    auto skel_ref = scn.find<SkeletonRef>();
    if(skel_ref && skel_ref->skeleton) {
        _resource->reference_skel = skel_ref->skeleton;
        _resource->setSkeleton(skel_ref->skeleton);

        sample_buffer.resize(_resource->reference_skel->boneCount());
    }
}

void DocActionGraph::onGuiToolbox(Editor* ed) {
    auto& action_graph = _resource;

    imguiResourceTreeCombo("reference", action_graph->reference_object, "so", [this, &action_graph](){
        if(action_graph->reference_object) {
            setReferenceObject(action_graph->reference_object.get());
        }
    });
    imguiResourceTreeCombo("skeleton", action_graph->reference_skel, "skl", [this, &action_graph](){
        action_graph->setSkeleton(action_graph->reference_skel);

        sample_buffer.resize(action_graph->reference_skel->boneCount());
    });

    ImGui::Text("Add state");
    if(ImGui::Button("Clip", ImVec2(ImGui::GetContentRegionAvailWidth(), .0f))) {
        auto act = action_graph->createAction<AnimFSMStateClip>("Clip state");
        ImVec2 new_pos;// = GraphEditGridScreenToPos(s_graph_edit_bb.Min + s_graph_edit_bb.Max * 0.5f);
        act->setEditorPos(gfxm::vec2(new_pos.x, new_pos.y));
        selected_action = act;
    }
    if(ImGui::Button("FSM", ImVec2(ImGui::GetContentRegionAvailWidth(), .0f))) {
        auto act = action_graph->createAction<AnimFSMStateFSM>("FSM state");
        ImVec2 new_pos;// = GraphEditGridScreenToPos(s_graph_edit_bb.Min + s_graph_edit_bb.Max * 0.5f);
        act->setEditorPos(gfxm::vec2(new_pos.x, new_pos.y));
        selected_action = act;
    }
    if(ImGui::Button("BlendTree", ImVec2(ImGui::GetContentRegionAvailWidth(), .0f))) {
        auto act = action_graph->createAction<AnimFSMStateBlendTree>("BlendTree state");
        ImVec2 new_pos;// = GraphEditGridScreenToPos(s_graph_edit_bb.Min + s_graph_edit_bb.Max * 0.5f);
        act->setEditorPos(gfxm::vec2(new_pos.x, new_pos.y));
        selected_action = act;
    }
    if(selected_action) {
        ImGuiExt::BeginInsetSegment(ImGui::ColorConvertFloat4ToU32(ImVec4(.3f, .15f, .0f, 1.f)));
        ImGui::Text("Selected state");

        char buf[256];
        memset(buf, 0, sizeof(buf));
        memcpy(buf, selected_action->getName().c_str(), selected_action->getName().size());
        if(ImGui::InputText("name", buf, sizeof(buf))) {
            action_graph->renameAction(selected_action, buf);
        }
        
        selected_action->onGuiToolbox();
        
        if(selected_action->getType() == rttr::type::get<AnimFSMStateFSM>()) {
            if(ImGui::Button(ICON_MDI_PENCIL " Edit FSM", ImVec2(ImGui::GetWindowContentRegionWidth(), .0f))) {
                setNestedWindow(new DocActionGraph());
            }
        } else if(selected_action->getType() == rttr::type::get<AnimFSMStateBlendTree>()) {
            if(ImGui::Button(ICON_MDI_PENCIL " Edit BlendTree", ImVec2(ImGui::GetWindowContentRegionWidth(), .0f))) {
                // TODO: Open nested document
            }
        }

        if(ImGui::Button("Delete action", ImVec2(ImGui::GetWindowContentRegionWidth(), .0f))) {
            action_graph->deleteAction(selected_action);
            selected_action = 0;
            selected_transition = 0;
        }

        ImGuiExt::EndInsetSegment();
    }
    ImGui::Separator();
    if(selected_transition) {
        ImGuiExt::BeginInsetSegment(ImGui::ColorConvertFloat4ToU32(ImVec4(.0f, .3f, .15f, 1.f)));
        ImGui::Text("Selected state");

        ImGui::Text(MKSTR(selected_transition->from->getName() << " -> " << selected_transition->to->getName()).c_str());
        ImGui::SameLine();
        if(ImGui::SmallButton(ICON_MDI_DELETE_EMPTY )) {
            action_graph->deleteTransition(selected_transition);
            selected_transition = 0;
        } else {
            ImGui::DragFloat("blend time", &selected_transition->blendTime);
            auto& conds = selected_transition->conditions;
            for(size_t i = 0; i < conds.size(); ++i) {
                auto& cond = conds[i];
                ImGui::PushItemWidth(70);
                if(ImGui::BeginCombo(MKSTR("###cond_id"<<i).c_str(), conds[i].param_name.c_str(), ImGuiComboFlags_NoArrowButton)) {
                    for(size_t j = 0; j < blackboard.count(); ++j) {
                        auto name = blackboard.getName(j);
                        if(ImGui::Selectable(name, j == cond.param_hdl)) {
                            cond.param_hdl = j;
                            conds[i].param_name = name;
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
                        0, "Param", AnimFSMTransition::CONDITION::LARGER, .0f
                    }
                );
            }
        }

        ImGuiExt::EndInsetSegment();
    }

    ImGui::Separator();
    
    animBlackboardGui(&blackboard);
}


void DocActionGraph::onResourceSet() {
    auto& action_graph = _resource;
    //action_graph->setBlackboard(&blackboard);

    if(action_graph->reference_object) {
        setReferenceObject(action_graph->reference_object.get());
    }
}