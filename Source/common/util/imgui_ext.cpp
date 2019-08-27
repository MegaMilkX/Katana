#include "imgui_ext.hpp"

#include <string>
#include <map>

#include "log.hpp"

namespace ImGuiExt {

static ImRect s_graph_edit_bb;
static ImVec2 s_graph_edit_offset;
static ImVec2 s_graph_edit_tmp_offset;
static ImVec2 s_graph_edit_grid_offset_plus_drag_delta;
static float s_graph_edit_zoom = 1.0f;

static const char* s_node_name = 0;
static ImVec2* s_node_pos_out = 0;
static ImRect s_node_bb;
static ImVec2 s_node_next_in_pos;
static ImVec2 s_node_next_out_pos;
static ImVec2 s_node_current_pos;
static const char* s_node_drag_id = 0;

static std::map<std::string, ImVec4> s_connection_map;

void BeginTreeNode(const char* name, ImVec2* pos, const ImVec2& size) {
    s_node_pos_out = pos;
    s_node_name = name;
    /*
    if(double_clicked) {
        *double_clicked = false;
    }*/
    ImVec2 text_size = ImGui::CalcTextSize(name);

    ImVec2 node_half_size = size * 0.5f;
    ImVec2 node_frame_min = (s_graph_edit_bb.Min + (*pos - node_half_size + s_graph_edit_grid_offset_plus_drag_delta) * s_graph_edit_zoom);
    ImVec2 node_frame_max = node_frame_min + size * s_graph_edit_zoom;
    s_node_bb = ImRect(node_frame_min, node_frame_max);
    s_node_next_in_pos = s_node_bb.Min + ImVec2(0, ImGui::GetTextLineHeight() * 3.0f);
    s_node_next_out_pos = ImVec2(s_node_bb.Max.x, s_node_bb.Min.y) + ImVec2(0, ImGui::GetTextLineHeight() * 3.0f);
    

/*
    if(highlight) {
        node_col = ImGui::GetColorU32(ImGuiCol_PlotHistogram);
    }*/
    //ImGui::PushClipRect(node_frame_min, node_frame_max, true);

    ImGuiWindowFlags flags = 0;

    

    


    ImGui::GetWindowDrawList()->ChannelsSplit(2);
}

void EndTreeNode() {
    bool clicked = false;

    ImU32 node_col = ImGui::GetColorU32(ImGuiCol_WindowBg, 1);

    if((s_node_drag_id == s_node_name) && ImGui::IsMouseDragging(0)) {
        *s_node_pos_out += ImGui::GetMouseDragDelta(0) / s_graph_edit_zoom;
        ImGui::ResetMouseDragDelta(0);
    }

    if(ImGui::IsMouseReleased(0)) {
        s_node_drag_id = 0;
    }

    bool hovered = false;
    bool held = false;
    bool pressed = ImGui::ButtonBehavior(ImRect(s_node_bb.Min, s_node_bb.Max), ImGui::GetID(s_node_name), &hovered, &held);

    if(ImGui::IsMouseHoveringRect(s_node_bb.Min, s_node_bb.Max, true)) {
        if(ImGui::IsMouseClicked(0)) {
            s_node_drag_id = s_node_name;
            clicked = true;
        }
        if(ImGui::IsMouseDoubleClicked(0)) {
            /*
            if(double_clicked) {
                *double_clicked = true;
            }*/
        }
    }

    ImGui::GetWindowDrawList()->ChannelsSetCurrent(0);

    ImGui::RenderFrame(
        s_node_bb.Min, s_node_bb.Max, 
        node_col, true, ImGui::GetStyle().FrameRounding
    );
    if(
        (s_node_drag_id == s_node_name || ImGui::IsMouseHoveringRect(s_node_bb.Min, s_node_bb.Max, true))
    ) {
        ImGui::GetWindowDrawList()->AddQuad(
            s_node_bb.Min,
            ImVec2(s_node_bb.Max.x, s_node_bb.Min.y), 
            s_node_bb.Max,
            ImVec2(s_node_bb.Min.x, s_node_bb.Max.y),
            ImGui::GetColorU32(ImGuiCol_Text, 1.0f), 2.0f
        );
    }
    ImGui::RenderText(s_node_bb.Min, s_node_name);

    ImGui::GetWindowDrawList()->ChannelsMerge();
}

bool TreeNodeIn(const char* name) {
    ImGui::GetWindowDrawList()->ChannelsSetCurrent(1);
    
    ImGui::GetWindowDrawList()->AddCircle(
        s_node_next_in_pos, 10 * s_graph_edit_zoom, ImGui::GetColorU32(ImGuiCol_Text), 4, 4.0f * s_graph_edit_zoom
    );
    ImGui::GetWindowDrawList()->AddText(s_node_next_in_pos + ImVec2(20, 0) * s_graph_edit_zoom - ImVec2(0, ImGui::GetTextLineHeight()) * 0.5f, ImGui::GetColorU32(ImGuiCol_Text), name);

    float new_node_height = s_node_bb.Max.y;
    if(new_node_height < s_node_next_in_pos.y) {
        new_node_height = s_node_next_in_pos.y + ImGui::GetTextLineHeight() * 2.0f;
    }
    s_node_bb.Max.y = new_node_height + ImGui::GetTextLineHeight() * 2.0f;

    s_node_current_pos = s_node_next_in_pos;

    s_node_next_in_pos += ImVec2(0, ImGui::GetTextLineHeight()) * 2;
    s_node_next_out_pos += ImVec2(0, ImGui::GetTextLineHeight()) * 2;
    
    return false;
}

bool TreeNodeOut(const char* name) {
    ImGui::GetWindowDrawList()->ChannelsSetCurrent(1);
    
    ImGui::GetWindowDrawList()->AddCircle(
        s_node_next_out_pos, 10 * s_graph_edit_zoom, ImGui::GetColorU32(ImGuiCol_Text), 4, 4.0f * s_graph_edit_zoom
    );
    ImGui::GetWindowDrawList()->AddText(s_node_next_in_pos + ImVec2(20, 0) * s_graph_edit_zoom - ImVec2(0, ImGui::GetTextLineHeight()) * 0.5f, ImGui::GetColorU32(ImGuiCol_Text), name);

    float new_node_height = s_node_bb.Max.y;
    if(new_node_height < s_node_next_in_pos.y) {
        new_node_height = s_node_next_in_pos.y + ImGui::GetTextLineHeight() * 2.0f;
    }
    s_node_bb.Max.y = new_node_height + ImGui::GetTextLineHeight() * 2.0f;

    s_node_current_pos = s_node_next_out_pos;

    s_node_next_in_pos += ImVec2(0, ImGui::GetTextLineHeight()) * 2;
    s_node_next_out_pos += ImVec2(0, ImGui::GetTextLineHeight()) * 2;

    return false;
}

void TreeNodeConnectionOut(const char* name) {
    s_connection_map[name].x = s_node_current_pos.x;
    s_connection_map[name].y = s_node_current_pos.y;
}
void TreeNodeConnectionIn(const char* out_name) {
    s_connection_map[out_name].z = s_node_current_pos.x;
    s_connection_map[out_name].w = s_node_current_pos.y;
}

void CommitTreeNodeConnections() {
    for(auto& kv : s_connection_map) {
        ImVec2 a(kv.second.x, kv.second.y);
        ImVec2 b(kv.second.z, kv.second.w);
        
        ImGui::GetWindowDrawList()->AddBezierCurve(
            a,
            a + ImVec2(100, 0) * s_graph_edit_zoom, b + ImVec2(-100, 0) * s_graph_edit_zoom,
            b,
            ImGui::GetColorU32(ImGuiCol_Text),
            4.0f * s_graph_edit_zoom, 50
        );
    }
}

void NodeConnection(const ImVec2& from, const ImVec2& to) {
    ImVec2 ref_pos = (s_graph_edit_bb.Min + (s_graph_edit_grid_offset_plus_drag_delta) * s_graph_edit_zoom);

    ImVec2 p0 = ref_pos + from * s_graph_edit_zoom;
    ImVec2 cp0 = ref_pos + (from + ImVec2(100,0)) * s_graph_edit_zoom;
    ImVec2 p1 = ref_pos + to * s_graph_edit_zoom;
    ImVec2 cp1 = ref_pos + (to + ImVec2(-100, 0)) * s_graph_edit_zoom;

    ImGui::GetWindowDrawList()->AddBezierCurve(
        p0,
        cp0, cp1,
        p1,
        ImGui::GetColorU32(ImGuiCol_Text),
        4.0f * s_graph_edit_zoom, 50
    );
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

    s_graph_edit_tmp_offset = ImGui::GetMouseDragDelta(2) / s_graph_edit_zoom;

    s_graph_edit_grid_offset_plus_drag_delta = s_graph_edit_offset + s_graph_edit_tmp_offset;
    if(ImGui::IsMouseReleased(2)) {
        s_graph_edit_offset += s_graph_edit_tmp_offset;
        s_graph_edit_tmp_offset = ImVec2();
    }
    

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

}