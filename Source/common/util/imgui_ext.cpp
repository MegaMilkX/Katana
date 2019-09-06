#include "imgui_ext.hpp"

#include <string>
#include <map>
#include <set>

#include "log.hpp"

namespace ImGuiExt {

struct NodeInOutCollection {
    std::vector<ImVec2> ins;
    std::vector<ImVec2> outs;
};

static ImGuiID s_grid_id;
static ImGuiID s_grid_scroll_id;
static ImRect s_graph_edit_bb;
static ImVec2 s_graph_edit_offset;
static ImVec2 s_graph_edit_tmp_offset;
static ImVec2 s_graph_edit_grid_offset_plus_drag_delta;
static float s_graph_edit_zoom = 1.0f;

static std::string s_node_name;
static ImVec2* s_node_pos_out = 0;
static ImRect s_node_bb;
static ImVec2 s_node_next_in_pos;
static ImVec2 s_node_next_out_pos;
static ImVec2 s_node_current_pos;
static ImGuiID s_node_drag_id = 0;
static bool s_node_selected = false;
static bool* s_node_clicked_ptr = 0;

static std::set<std::pair<void*, void*>> s_connections;
static std::vector<NodeInOutCollection> s_node_cache;
enum NEW_CONN_TYPE {
    NEW_CONN_NONE,
    NEW_CONN_OUT,
    NEW_CONN_IN
};
static ImGuiID s_new_conn_grid_owner = 0;
static NEW_CONN_TYPE s_new_conn_type = NEW_CONN_NONE;
static void* s_potential_connection_origin_id = 0;
static ImVec2 s_new_connection_origin;

static std::map<void*, ImVec2> s_node_point_coords;

static ImVec2 GridPosToScreen(const ImVec2& pos) {
    return s_graph_edit_bb.Min + (pos + s_graph_edit_grid_offset_plus_drag_delta) * s_graph_edit_zoom;
}

static ImVec2 GridScreenToPos(const ImVec2& pos) {
    ImVec2 r = pos;
    r = r - s_graph_edit_bb.Min;
    r = r / s_graph_edit_zoom;
    r = r - s_graph_edit_offset;
    return r;
}

void BeginTreeNode(const char* name, ImVec2* pos, bool* clicked, bool selected, const ImVec2& size) {
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
    
    s_node_selected = selected;
    s_node_clicked_ptr = clicked;

/*
    if(highlight) {
        node_col = ImGui::GetColorU32(ImGuiCol_PlotHistogram);
    }*/
    //ImGui::PushClipRect(node_frame_min, node_frame_max, true);

    ImGuiWindowFlags flags = 0;

    

    


    ImGui::GetWindowDrawList()->ChannelsSplit(2);
    s_node_cache.emplace_back(NodeInOutCollection{});
}

void EndTreeNode() {
    bool clicked = false;

    ImU32 node_col = ImGui::GetColorU32(ImGuiCol_WindowBg, 1);

    if((s_node_drag_id == ImGui::GetID(s_node_name.c_str())) && ImGui::IsMouseDragging(0)) {
        *s_node_pos_out += ImGui::GetMouseDragDelta(0) / s_graph_edit_zoom;
        ImGui::ResetMouseDragDelta(0);
    }

    if(ImGui::IsMouseReleased(0)) {
        s_node_drag_id = 0;
    }

    bool hovered = false;
    bool held = false;
    bool pressed = ImGui::ButtonBehavior(ImRect(s_node_bb.Min, s_node_bb.Max), ImGui::GetID(s_node_name.c_str()), &hovered, &held);

    if(ImGui::IsMouseHoveringRect(s_node_bb.Min, s_node_bb.Max, true)) {
        if(ImGui::IsMouseDragging(0)) {
            s_new_conn_type = NEW_CONN_NONE;
        }
        if(ImGui::IsMouseClicked(0)) {
            s_node_drag_id = ImGui::GetID(s_node_name.c_str());
            clicked = true;
            if(s_node_clicked_ptr) {
                *s_node_clicked_ptr = true;
            }
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
        node_col, true, 10.0f * s_graph_edit_zoom //ImGui::GetStyle().FrameRounding
    );
    if(
        (s_node_selected || s_node_drag_id == ImGui::GetID(s_node_name.c_str()) || ImGui::IsMouseHoveringRect(s_node_bb.Min, s_node_bb.Max, true))
    ) {
        ImGui::GetWindowDrawList()->AddRect(
            s_node_bb.Min, s_node_bb.Max, ImGui::GetColorU32(ImGuiCol_Text, 1.0f),
            10.0f * s_graph_edit_zoom, 15, 2.0f
        );
    }
    ImGui::RenderText(ImGui::GetStyle().WindowPadding * s_graph_edit_zoom + s_node_bb.Min, s_node_name.c_str());

    ImGui::GetWindowDrawList()->ChannelsMerge();

    s_node_selected = false;
    s_node_clicked_ptr = 0;
}

void* TreeNodeIn(const char* name, void* user_id, void* other_user_id) {
    bool new_connection_occured = false;
    ImGui::GetWindowDrawList()->ChannelsSetCurrent(1);

    if(ImGui::IsMouseHoveringRect(s_node_next_in_pos + ImVec2(-10, -10) * s_graph_edit_zoom, s_node_next_in_pos + ImVec2(10, 10) * s_graph_edit_zoom, true)) {
        if(ImGui::IsMouseClicked(0)) {
            if(s_new_conn_type == NEW_CONN_NONE || s_new_conn_type == NEW_CONN_IN) {
                s_new_conn_grid_owner = s_grid_id;
                s_new_conn_type = NEW_CONN_IN;
                s_new_connection_origin = GridScreenToPos(s_node_next_in_pos);
                s_potential_connection_origin_id = user_id;
            } else if (s_new_conn_type == NEW_CONN_OUT && s_new_conn_grid_owner == s_grid_id) {
                s_new_conn_type = NEW_CONN_NONE;
                new_connection_occured = true;
                other_user_id = s_potential_connection_origin_id;
            }
        }
        ImGui::GetWindowDrawList()->AddCircleFilled(
            s_node_next_in_pos, 9.5f * s_graph_edit_zoom, ImGui::GetColorU32(ImGuiCol_Text), 4
        );
    } else {
        ImGui::GetWindowDrawList()->AddCircle(
            s_node_next_in_pos, 8 * s_graph_edit_zoom, ImGui::GetColorU32(ImGuiCol_Text), 4, 3.0f * s_graph_edit_zoom
        );
    }
    ImGui::GetWindowDrawList()->AddText(s_node_next_in_pos + ImVec2(20, 0) * s_graph_edit_zoom - ImVec2(0, ImGui::GetTextLineHeight()) * 0.5f, ImGui::GetColorU32(ImGuiCol_Text), name);

    float new_node_height = s_node_bb.Max.y;
    if(new_node_height < s_node_next_in_pos.y) {
        new_node_height = s_node_next_in_pos.y + ImGui::GetTextLineHeight() * 2.0f;
    }

    s_node_current_pos = s_node_next_in_pos;

    s_node_next_in_pos += ImVec2(0, ImGui::GetTextLineHeight()) * 2;
    s_node_next_out_pos += ImVec2(0, ImGui::GetTextLineHeight()) * 2;

    s_node_bb.Max.y = s_node_next_in_pos.y;

    if(user_id) {
        s_node_point_coords[user_id] = s_node_current_pos;
    }
    s_node_cache.back().ins.emplace_back(s_node_current_pos);

    if(user_id && other_user_id) {
        s_connections.insert(std::make_pair(other_user_id, user_id));
    }
    
    if(new_connection_occured) {
        return other_user_id;
    } else {
        return 0;
    }
}

void* TreeNodeOut(const char* name, void* user_id, void* other_user_id) {
    bool new_connection_occured = false;
    ImGui::GetWindowDrawList()->ChannelsSetCurrent(1);
    
    if(ImGui::IsMouseHoveringRect(s_node_next_out_pos + ImVec2(-10, -10) * s_graph_edit_zoom, s_node_next_out_pos + ImVec2(10, 10) * s_graph_edit_zoom, true)) {
        if(ImGui::IsMouseClicked(0)) {
            if(s_new_conn_type == NEW_CONN_NONE || s_new_conn_type == NEW_CONN_OUT) {
                s_new_conn_grid_owner = s_grid_id;
                s_new_conn_type = NEW_CONN_OUT;
                s_new_connection_origin = GridScreenToPos(s_node_next_out_pos);
                s_potential_connection_origin_id = user_id;
            } else if(s_new_conn_type == NEW_CONN_IN && s_new_conn_grid_owner == s_grid_id) {
                s_new_conn_type = NEW_CONN_NONE;
                new_connection_occured = true;
                other_user_id = s_potential_connection_origin_id;
            }
        }
        ImGui::GetWindowDrawList()->AddCircleFilled(
            s_node_next_out_pos, 9.5f * s_graph_edit_zoom, ImGui::GetColorU32(ImGuiCol_Text), 4
        );
    } else {
        ImGui::GetWindowDrawList()->AddCircle(
            s_node_next_out_pos, 8 * s_graph_edit_zoom, ImGui::GetColorU32(ImGuiCol_Text), 4, 3.0f * s_graph_edit_zoom
        );
    }
    ImGui::GetWindowDrawList()->AddText(s_node_next_in_pos + ImVec2(20, 0) * s_graph_edit_zoom - ImVec2(0, ImGui::GetTextLineHeight()) * 0.5f, ImGui::GetColorU32(ImGuiCol_Text), name);

    float new_node_height = s_node_bb.Max.y;
    if(new_node_height < s_node_next_in_pos.y) {
        new_node_height = s_node_next_in_pos.y + ImGui::GetTextLineHeight() * 2.0f;
    }

    s_node_current_pos = s_node_next_out_pos;

    s_node_next_in_pos += ImVec2(0, ImGui::GetTextLineHeight()) * 2;
    s_node_next_out_pos += ImVec2(0, ImGui::GetTextLineHeight()) * 2;

    s_node_bb.Max.y = s_node_next_out_pos.y;

    if(user_id) {
        s_node_point_coords[user_id] = s_node_current_pos;
    }
    s_node_cache.back().outs.emplace_back(s_node_current_pos);

    if(user_id && other_user_id) {
        s_connections.insert(std::make_pair(user_id, other_user_id));
    }

    if(new_connection_occured) {
        return other_user_id;
    } else {
        return 0;
    }
}

void CommitTreeNodeConnections() {
    for(auto& it : s_connections) {
        ImVec2 a = s_node_point_coords[(it).first];
        ImVec2 b = s_node_point_coords[(it).second];

        ImGui::GetWindowDrawList()->AddBezierCurve(
            a,
            a + ImVec2(100, 0) * s_graph_edit_zoom, b + ImVec2(-100, 0) * s_graph_edit_zoom,
            b,
            ImGui::GetColorU32(ImGuiCol_Text),
            3.0f * s_graph_edit_zoom, 50
        );
    }
}

void TreeNodeConnection(size_t node_from, size_t node_to, size_t out_n, size_t in_n) {
    if(node_from >= s_node_cache.size()) return;
    if(node_to >= s_node_cache.size()) return;
    auto& inouts_a = s_node_cache[node_from];
    auto& inouts_b = s_node_cache[node_to];
    if(out_n >= inouts_a.outs.size()) return;
    if(in_n >= inouts_b.ins.size()) return;
    ImVec2 a = inouts_a.outs[out_n];
    ImVec2 b = inouts_b.ins[in_n];

    ImGui::GetWindowDrawList()->AddBezierCurve(
        a,
        a + ImVec2(100, 0) * s_graph_edit_zoom, b + ImVec2(-100, 0) * s_graph_edit_zoom,
        b,
        ImGui::GetColorU32(ImGuiCol_Text),
        3.0f * s_graph_edit_zoom, 50
    );
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
        3.0f * s_graph_edit_zoom, 50
    );
}

bool BeginGridView(const char* id) {
    if(ImGui::BeginChild(id)) {
        s_grid_id = ImGui::GetID(id);
        std::string name = id;
        auto window = ImGui::GetCurrentWindow();

        auto bb = window->ContentsRegionRect;
        bb = window->WorkRect;
        bb = window->InnerRect;
        s_graph_edit_bb = bb;
        ImVec2 size = ImVec2(bb.Max.x - bb.Min.x, bb.Max.y - bb.Min.y);

        ImGuiIO& io = ImGui::GetIO();
        ImVec2 cursor_pos = io.MousePos;
        cursor_pos.x = cursor_pos.x - bb.Min.x;
        cursor_pos.y = cursor_pos.y - bb.Min.y;

        bool hovered = ImGui::IsWindowHovered();

        if(io.MouseWheel != .0f && hovered) {
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

        if(hovered && ImGui::IsMouseDown(2)) {
            s_grid_scroll_id = s_grid_id;
        }
        
        if(s_grid_scroll_id == s_grid_id) {
            s_graph_edit_tmp_offset = ImGui::GetMouseDragDelta(2) / s_graph_edit_zoom;
        }

        s_graph_edit_grid_offset_plus_drag_delta = s_graph_edit_offset + s_graph_edit_tmp_offset;
        if(s_grid_scroll_id == s_grid_id && ImGui::IsMouseReleased(2)) {
            s_graph_edit_offset += s_graph_edit_tmp_offset;
            s_grid_scroll_id = 0;
        }
        s_graph_edit_tmp_offset = ImVec2();

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
    CommitTreeNodeConnections();

    if(s_new_conn_type != NEW_CONN_NONE && s_new_conn_grid_owner == s_grid_id) {
        ImVec2 a;
        ImVec2 b;
        if(s_new_conn_type == NEW_CONN_OUT) {
            a = s_new_connection_origin;
            b = GridScreenToPos(ImGui::GetMousePos());
        } else if(s_new_conn_type == NEW_CONN_IN) {
            a = GridScreenToPos(ImGui::GetMousePos());
            b = s_new_connection_origin;
        }

        NodeConnection(a, b);

        if(ImGui::IsMouseClicked(1)) {
            s_new_conn_type = NEW_CONN_NONE;
        }
    }

    ImGui::EndChild();
    s_node_cache.clear();
    s_connections.clear();
    s_node_point_coords.clear();

    ImGui::RenderText(s_graph_edit_bb.Min, MKSTR("Zoom: " << s_graph_edit_zoom).c_str());
    ImGui::RenderText(s_graph_edit_bb.Min + ImVec2(0, ImGui::GetTextLineHeight() * 1), MKSTR("x: " << s_graph_edit_grid_offset_plus_drag_delta.x).c_str());
    ImGui::RenderText(s_graph_edit_bb.Min + ImVec2(0, ImGui::GetTextLineHeight() * 2), MKSTR("y: " << s_graph_edit_grid_offset_plus_drag_delta.y).c_str());
}

}