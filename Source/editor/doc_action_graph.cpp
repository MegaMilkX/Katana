#include "doc_action_graph.hpp"

#include "../common/util/imgui_helpers.hpp"
#include "../common/util/imgui_ext.hpp"

DocActionGraph::DocActionGraph() {
    viewport.camMode(GuiViewport::CAM_ORBIT);
    viewport.enableDebugDraw(false);

    onResourceSet();
}

static ImRect s_graph_edit_bb;
static ImVec2 s_graph_edit_offset;
static ImVec2 s_graph_edit_tmp_offset;
static ImVec2 s_graph_edit_grid_offset_plus_drag_delta;
static float s_graph_edit_zoom = 1.0f;

bool pointVsLine(const gfxm::vec2& from, const gfxm::vec2& to, const gfxm::vec2& point, float threshold = 0.1f) {
    float line_len = gfxm::length(to - from);
    float len_to_a = gfxm::length(point - from);
    float len_to_b = gfxm::length(point - to);
    float len_sum = len_to_a + len_to_b;
    return (len_sum <= line_len + threshold);
}

ImVec2 GraphEditGridPosToScreen(const ImVec2& pos) {
    return s_graph_edit_bb.Min + (pos + s_graph_edit_grid_offset_plus_drag_delta) * s_graph_edit_zoom;
}

ImVec2 GraphEditGridScreenToPos(const ImVec2& pos) {
    ImVec2 r = pos;
    r = r - s_graph_edit_bb.Min;
    r = r / s_graph_edit_zoom;
    r = r - s_graph_edit_offset;
    return r;
}

bool TransitionLine(const ImVec2& from, const ImVec2& to, bool selected = false) {
    bool clicked = false;

    ImVec2 diff = to - from;
    gfxm::vec2 p(diff.y, -diff.x);
    p = gfxm::normalize(p);
    p = p * 10.0f;
    ImVec2 from_(from.x + p.x, from.y + p.y);
    ImVec2 to_(to.x + p.x, to.y + p.y);
    ImVec2 center_ = to_ - from_;
    center_ = from_ + center_ * 0.45f;
    ImVec2 triangle[] = {
        GraphEditGridPosToScreen(center_ + ImVec2(p.x, p.y) * 0.5f),
        GraphEditGridPosToScreen(center_ - ImVec2(p.x, p.y) * 0.5f),
        GraphEditGridPosToScreen(center_ + (to_ - from_) * 0.05f)
    };

    ImVec2 graph_space_mouse_pos = GraphEditGridScreenToPos(ImGui::GetMousePos());
    bool is_mouse_over = pointVsLine(gfxm::vec2(from_.x, from_.y), gfxm::vec2(to_.x, to_.y), gfxm::vec2(graph_space_mouse_pos.x, graph_space_mouse_pos.y), 0.5f);
    if(is_mouse_over) {
        if(ImGui::IsMouseClicked(0)) {
            clicked = true;
        }
    }

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImU32 line_col = ImGui::GetColorU32(ImGuiCol_Text);
    if(is_mouse_over || selected) {
        line_col = ImGui::GetColorU32(ImGuiCol_PlotHistogram);
    }
    window->DrawList->AddLine(
        GraphEditGridPosToScreen(from_), 
        GraphEditGridPosToScreen(to_), 
        line_col, 
        3.0f
    );
    window->DrawList->AddTriangleFilled(
        triangle[0], triangle[1], triangle[2], line_col
    );

    return clicked;
}

static const char* s_node_drag_id = 0;

static const int NODE_FLAG_CLIP = 1;
static const int NODE_FLAG_FSM = 2;
static const int NODE_FLAG_BLEND_TREE = 4;
static const int NODE_FLAG_HIGHLIGHT = 8;
static const int NODE_FLAG_SELECTED = 16;

bool Node(const char* id, ImVec2& pos, const ImVec2& node_size, int node_flags, bool* double_clicked = 0) {
    if(double_clicked) {
        *double_clicked = false;
    }
    bool clicked = false;
    ImVec2 text_size = ImGui::CalcTextSize(id);

    bool selected = (node_flags & NODE_FLAG_SELECTED);
    bool highlight = (node_flags & NODE_FLAG_HIGHLIGHT);

    ImVec2 node_half_size = node_size * 0.5f;
    ImVec2 node_frame_min = (s_graph_edit_bb.Min + (pos - node_half_size + s_graph_edit_grid_offset_plus_drag_delta) * s_graph_edit_zoom);
    ImVec2 node_frame_max = node_frame_min + node_size * s_graph_edit_zoom;
    ImU32 node_col = ImGui::ColorConvertFloat4ToU32(ImVec4(.0f, .0f, .0f, 1.f));// ImGui::GetColorU32(ImGuiCol_WindowBg, 1);

    if(highlight) {
        node_col = ImGui::ColorConvertFloat4ToU32(ImVec4(209 / 255.0f, 99 / 255.0f, 44 / 255.0f, 1.0f)); //ImGui::GetColorU32(ImGuiCol_PlotHistogram);
    }
    //ImGui::PushClipRect(node_frame_min, node_frame_max, true);

    ImGuiWindowFlags flags = 0;

    ImGui::RenderFrame(
        node_frame_min, node_frame_max, 
        node_col, true, ImGui::GetStyle().FrameRounding
    );

    if((s_node_drag_id == id) && ImGui::IsMouseDragging(0)) {
        pos += ImGui::GetMouseDragDelta(0) / s_graph_edit_zoom;
        ImGui::ResetMouseDragDelta(0);
    }

    if(ImGui::IsMouseReleased(0)) {
        s_node_drag_id = 0;
    }

    bool hovered = false;
    bool held = false;
    bool pressed = ImGui::ButtonBehavior(ImRect(node_frame_min, node_frame_max), ImGui::GetID(id), &hovered, &held);

    if(ImGui::IsMouseHoveringRect(node_frame_min, node_frame_max, true)) {
        if(ImGui::IsMouseClicked(0)) {
            s_node_drag_id = id;
            clicked = true;
        }
        if(ImGui::IsMouseDoubleClicked(0)) {
            if(double_clicked) {
                *double_clicked = true;
            }
        }
    }

    if(
        (s_node_drag_id == id || ImGui::IsMouseHoveringRect(node_frame_min, node_frame_max, true))
        || (selected)
    ) {
        ImGui::GetWindowDrawList()->AddQuad(
            node_frame_min,
            ImVec2(node_frame_max.x, node_frame_min.y), 
            node_frame_max,
            ImVec2(node_frame_min.x, node_frame_max.y),
            ImGui::GetColorU32(ImGuiCol_Text, 1.0f), 2.0f
        );
    }

    ImGui::RenderText((node_frame_min + (node_size * s_graph_edit_zoom * 0.5f) - (text_size * 0.5f)), id);

    const char* icon_str = "";
    if(node_flags & NODE_FLAG_CLIP)
        icon_str = ICON_MDI_RUN;
    else if(node_flags & NODE_FLAG_FSM)
        icon_str = ICON_MDI_SETTINGS;
    else if(node_flags & NODE_FLAG_BLEND_TREE)
        icon_str = "BT";

    ImGui::RenderText(node_frame_min, icon_str);

    //ImGui::PopClipRect();
    return clicked;
}

bool BeginGridView(const char* id) {
    if(ImGui::BeginChild(id)) {
        std::string name = id;
        auto window = ImGui::GetCurrentWindow();

        auto bb = s_graph_edit_bb = window->ContentsRegionRect;
        ImVec2 size = ImVec2(bb.Max.x - bb.Min.x, bb.Max.y - bb.Min.y);

        ImGuiIO& io = ImGui::GetIO();
        ImVec2 cursor_pos = io.MousePos;
        cursor_pos.x = cursor_pos.x - bb.Min.x;
        cursor_pos.y = cursor_pos.y - bb.Min.y;

        if(io.MouseWheel != .0f && ImGui::IsMouseHoveringRect(bb.Min, bb.Max)) {
            ImVec2 observed_size_before_zoom_change = size / s_graph_edit_zoom;
            s_graph_edit_zoom += io.MouseWheel * 0.1f;
            if(s_graph_edit_zoom < 0.1f) {
                s_graph_edit_zoom = 0.1f;
            }
            ImVec2 observed_size_after_zoom_change = size / s_graph_edit_zoom;
            ImVec2 observed_box_size_delta = observed_size_before_zoom_change - observed_size_after_zoom_change;
            ImVec2 cursor_pos_factor = cursor_pos / size;
            ImVec2 zoom_pos_adjustment = observed_box_size_delta * cursor_pos_factor;
            s_graph_edit_offset -= zoom_pos_adjustment;
        }

        s_graph_edit_tmp_offset = ImGui::GetMouseDragDelta(2) / s_graph_edit_zoom;

        s_graph_edit_grid_offset_plus_drag_delta = s_graph_edit_offset + s_graph_edit_tmp_offset;
        if(ImGui::IsMouseReleased(2)) {
            s_graph_edit_offset += s_graph_edit_tmp_offset;
            s_graph_edit_tmp_offset = ImVec2();
        }

        ImGui::PushClipRect(bb.Min, bb.Max, false);
        ImGui::RenderFrame(
            bb.Min, bb.Max, 
            ImGui::GetColorU32(ImGuiCol_FrameBg, 1), true, ImGui::GetStyle().FrameRounding
        );

        float grid_step = 100.0f * s_graph_edit_zoom;
        float ax = s_graph_edit_grid_offset_plus_drag_delta.x / 100.0f;
        ax = ax - (int)ax;
        float ay = s_graph_edit_grid_offset_plus_drag_delta.y / 100.0f;
        ay = ay - (int)ay;
        ax *= grid_step;
        ay *= grid_step;

        ImU32 text_color = ImGui::GetColorU32(ImGuiCol_TextDisabled);
        for(float y = bb.Min.y + ay; y <= bb.Max.y; y += grid_step) {
            window->DrawList->AddLine(
                ImVec2(bb.Min.x, y),
                ImVec2(bb.Max.x, y),
                text_color
            );
            //window->DrawList->AddText(ImVec2(bb.Min.x, y), text_color, MKSTR(v).c_str());
        }
        for(float x = bb.Min.x + ax; x <= bb.Max.x; x += grid_step) {
            window->DrawList->AddLine(
                ImVec2(x, bb.Min.y),
                ImVec2(x, bb.Max.y),
                text_color
            );
        }

        ImGui::SetWindowFontScale(s_graph_edit_zoom);
/*
        static ImVec2 node_pos_a(0,0), node_pos_b(-200, 150), node_pos_c(200,100);
        
        TransitionLine(node_pos_a, node_pos_b);
        TransitionLine(node_pos_b, node_pos_a);

        TransitionLine(node_pos_b, node_pos_c);
        
        Node("Action test", node_pos_a, ImVec2(200, 50));
        Node("Action test2", node_pos_b, ImVec2(200, 50));
        Node("Some node", node_pos_c, ImVec2(200, 50));
*/
        ImGui::PopClipRect();

        

        return true;
    } else {
        return false;
    }
}

void EndGridView() {
    ImGui::EndChild();

    ImGui::RenderText(s_graph_edit_bb.Min, MKSTR("Zoom: " << s_graph_edit_zoom).c_str());
    ImGui::RenderText(s_graph_edit_bb.Min + ImVec2(0, ImGui::GetTextLineHeight() * 1), MKSTR("x: " << s_graph_edit_grid_offset_plus_drag_delta.x).c_str());
    ImGui::RenderText(s_graph_edit_bb.Min + ImVec2(0, ImGui::GetTextLineHeight() * 2), MKSTR("y: " << s_graph_edit_grid_offset_plus_drag_delta.y).c_str());
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

const char* condTypeToCStr(AnimFSMTransition::CONDITION cond) {
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
        ImVec2 new_pos = GraphEditGridScreenToPos(s_graph_edit_bb.Min + s_graph_edit_bb.Max * 0.5f);
        act->setEditorPos(gfxm::vec2(new_pos.x, new_pos.y));
        selected_action = act;
    }
    if(ImGui::Button("FSM", ImVec2(ImGui::GetContentRegionAvailWidth(), .0f))) {
        auto act = action_graph->createAction<AnimFSMStateFSM>("FSM state");
        ImVec2 new_pos = GraphEditGridScreenToPos(s_graph_edit_bb.Min + s_graph_edit_bb.Max * 0.5f);
        act->setEditorPos(gfxm::vec2(new_pos.x, new_pos.y));
        selected_action = act;
    }
    if(ImGui::Button("BlendTree", ImVec2(ImGui::GetContentRegionAvailWidth(), .0f))) {
        auto act = action_graph->createAction<AnimFSMStateBlendTree>("BlendTree state");
        ImVec2 new_pos = GraphEditGridScreenToPos(s_graph_edit_bb.Min + s_graph_edit_bb.Max * 0.5f);
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
    action_graph->setBlackboard(&blackboard);

    if(action_graph->reference_object) {
        setReferenceObject(action_graph->reference_object.get());
    }
}