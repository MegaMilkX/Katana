#include "doc_action_graph.hpp"

#include "../common/util/imgui_helpers.hpp"

DocActionGraph::DocActionGraph() {

}

DocActionGraph::DocActionGraph(std::shared_ptr<ResourceNode>& node) {
    setResourceNode(node);
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

bool Node(const char* id, ImVec2& pos, const ImVec2& node_size, bool selected = false, bool highlight = false, bool* double_clicked = 0) {
    if(double_clicked) {
        *double_clicked = false;
    }
    bool clicked = false;
    ImVec2 text_size = ImGui::CalcTextSize(id);

    ImVec2 node_half_size = node_size * 0.5f;
    ImVec2 node_frame_min = (s_graph_edit_bb.Min + (pos - node_half_size + s_graph_edit_grid_offset_plus_drag_delta) * s_graph_edit_zoom);
    ImVec2 node_frame_max = node_frame_min + node_size * s_graph_edit_zoom;
    ImU32 node_col = ImGui::GetColorU32(ImGuiCol_WindowBg, 1);

    if(highlight) {
        node_col = ImGui::GetColorU32(ImGuiCol_PlotHistogram);
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

    //ImGui::PopClipRect();
    return clicked;
}

bool BeginGridView(const char* id) {
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

    if(ImGui::IsMouseReleased(2)) {
        s_graph_edit_offset += s_graph_edit_tmp_offset;
        s_graph_edit_tmp_offset = ImVec2();
    }
    s_graph_edit_tmp_offset = ImGui::GetMouseDragDelta(2) / s_graph_edit_zoom;

    s_graph_edit_grid_offset_plus_drag_delta = s_graph_edit_offset + s_graph_edit_tmp_offset;

    if(ImGui::BeginChild(id)) {
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
            if(Node(a->getName().c_str(), node_pos, ImVec2(200, 50), selected_action == a, a == action_graph->getEntryAction(), &dbl_clicked)) {
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
}

void DocActionGraph::onGuiToolbox(Editor* ed) {
    auto& action_graph = _resource;
    if(ImGui::Button("Add action")) {
        auto act = action_graph->createAction();
        ImVec2 new_pos = GraphEditGridScreenToPos(s_graph_edit_bb.Min + s_graph_edit_bb.Max * 0.5f);
        act->setEditorPos(gfxm::vec2(new_pos.x, new_pos.y));
        selected_action = act;
    }
    if(selected_action) {
        char buf[256];
        memset(buf, 0, sizeof(buf));
        memcpy(buf, selected_action->getName().c_str(), selected_action->getName().size());
        if(ImGui::InputText("name", buf, sizeof(buf))) {
            action_graph->renameAction(selected_action, buf);
        }
        imguiResourceTreeCombo("anim", selected_action->anim, "anm", [](){

        });
        if(ImGui::Button("Delete action")) {
            action_graph->deleteAction(selected_action);
            selected_action = 0;
            selected_transition = 0;
        }
    }
    if(selected_transition) {
        ImGui::Separator();
        ImGui::Text("Selected transition");
        ImGui::Text(MKSTR(selected_transition->from->getName() << " -> " << selected_transition->to->getName()).c_str());
        ImGui::Text("Conditions");
        ImGui::Text("Param"); ImGui::SameLine();
        ImGui::Text(">=");
        ImGui::SameLine();
        float v = .0f;
        ImGui::DragFloat("###cond_value", &v, .01f);
        if(ImGui::BeginCombo("###add_cond", "<add condition>")) {
            auto& params = action_graph->getParams();
            for(size_t i = 0; i < params.paramCount(); ++i) {
                auto& p = params.getParam(i);
                if(ImGui::Selectable(p.name.c_str(), false)) {

                }
            }
            ImGui::EndCombo();
        }
        if(ImGui::Button("Delete transition")) {
            action_graph->deleteTransition(selected_transition);
            selected_transition = 0;
        }
    }

    ImGui::Separator();
    ImGui::Text("Parameters");
    auto& params = action_graph->getParams();
    for(size_t i = 0; i < params.paramCount(); ++i) {
        auto& p = params.getParam(i);
        ImGui::Text(p.name.c_str());
        ImGui::SameLine();
        ImGui::DragFloat(MKSTR("###" << &p).c_str(), &p.value, 0.01f);
    }

    if(ImGui::SmallButton(ICON_MDI_PLUS)) {
        action_graph->getParams().createParam("Param");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("Add parameter");
        ImGui::EndTooltip();
    }
}