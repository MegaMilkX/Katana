#include "imgui_ext.hpp"

#include <string>
#include <map>
#include <set>
#include <unordered_map>

#include "log.hpp"

namespace ImGuiExt {


bool InputText(const char* name, std::string& value, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data) {
    char buf[256];
    memset(buf, 0, sizeof(buf));
    memcpy(buf, value.c_str(), value.size());
    if(ImGui::InputText(name, buf, sizeof(buf), flags, callback, user_data)) {
        value = buf;
        return true;
    } else {
        return false;
    }
}


struct NodeInOutCollection {
    std::vector<ImVec2> ins;
    std::vector<ImVec2> outs;
};

struct GridBoxData {
    ImGuiID id;
    ImGuiID scroll_id;
    ImRect  bb;
    ImVec2  offset;
    ImVec2  tmp_offset;
    ImVec2  offset_plus_drag_delta;
    float   zoom = 1.0f;
};
std::unordered_map<std::string, GridBoxData> s_grid_data;
static GridBoxData* cur_grid_data = 0;

static std::string s_node_name;
static ImVec2* s_node_pos_out = 0;
static ImRect s_node_bb;
static ImVec2 s_node_next_in_pos;
static ImVec2 s_node_next_out_pos;
static ImVec2 s_node_current_pos;
static ImGuiID s_node_drag_id = 0;
static bool s_node_selected = false;
static bool* s_node_clicked_ptr = 0;
static ImU32 s_node_col;

struct Connection {
    uint64_t a = -1;
    uint64_t b = -1;
    ImU32    color;

    bool operator<(const Connection& other) const {
        if(a == other.a) {
            return b < other.b;
        } else {
            return a < other.a;
        }
    }
};
static std::set<Connection> s_connections;
static std::vector<NodeInOutCollection> s_node_cache;
enum NEW_CONN_TYPE {
    NEW_CONN_NONE,
    NEW_CONN_OUT,
    NEW_CONN_IN
};
static ImGuiID s_new_conn_grid_owner = 0;
static NEW_CONN_TYPE s_new_conn_type = NEW_CONN_NONE;
static int64_t s_potential_connection_origin_id = 0;
static ImVec2 s_new_connection_origin;

static std::map<int64_t, ImVec2> s_node_point_coords;

static ImVec2 GridPosToScreen(const ImVec2& pos, const GridBoxData& data) {
    return data.bb.Min + (pos + data.offset_plus_drag_delta) * data.zoom;
}

static ImVec2 GridScreenToPos(const ImVec2& pos, const GridBoxData& data) {
    ImVec2 r = pos;
    r = r - data.bb.Min;
    r = r / data.zoom;
    r = r - data.offset;
    return r;
}

void GridLine(const char* label, const ImVec2& from, const ImVec2& to, ImU32 col) {
    auto& data = *cur_grid_data;
    ImVec2 center_offset_half = (data.bb.Max - data.bb.Min) * .5f;
    ImVec2 f = (data.bb.Min + center_offset_half + (from + data.offset_plus_drag_delta) * data.zoom);
    ImVec2 t = (data.bb.Min + center_offset_half + (to + data.offset_plus_drag_delta) * data.zoom);
    ImGui::GetWindowDrawList()->AddLine(f, t, col, 1.0f * data.zoom);
}


bool GridPoint(const char* label, ImVec2& pos, float radius, ImU32 col, bool* clicked, bool* selected) {
    bool being_used = false;
    auto& data = *cur_grid_data;
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 center_offset_half = (data.bb.Max - data.bb.Min) * .5f;
    ImVec2 p = (data.bb.Min + center_offset_half + (pos + data.offset_plus_drag_delta) * data.zoom);
    bool hovering = false;
    if(ImGui::IsMouseHoveringRect(p - ImVec2(radius, radius), p + ImVec2(radius, radius))) {
        hovering = true;
        if(ImGui::IsMouseDown(0) && !ImGui::IsMouseDragging(0)) {
            s_node_drag_id = ImGui::GetID(label);
            being_used = true;
        }
    }
    if(ImGui::IsMouseReleased(0)) {
        s_node_drag_id = 0;
    }
    if((s_node_drag_id == ImGui::GetID(label)) && ImGui::IsMouseDragging(0)) {
        ImVec2 mdd = ImGui::GetMouseDragDelta(0);
        pos += mdd / data.zoom;
        ImGui::ResetMouseDragDelta(0);
        being_used = true;
    }
    if(hovering || being_used) {
        ImGui::GetWindowDrawList()->AddCircle(
            p, radius * data.zoom, col, 12, 3.0f * data.zoom
        );
    } else {
        ImGui::GetWindowDrawList()->AddCircleFilled(
            p, radius * data.zoom, col, 12
        );
    }
    ImGui::GetWindowDrawList()->AddText(
        p + ImVec2(10, 10) * data.zoom - ImVec2(0, ImGui::GetTextLineHeight()) * 0.5f, 
        ImGui::GetColorU32(ImGuiCol_Text), label
    );
    return being_used;
}

void BeginTreeNode(const char* name, ImVec2* pos, bool* clicked, bool selected, const ImVec2& size, ImU32 col) {
    auto& data = *cur_grid_data;

    s_node_pos_out = pos;
    s_node_name = name;
    /*
    if(double_clicked) {
        *double_clicked = false;
    }*/
    ImVec2 text_size = ImGui::CalcTextSize(name);

    ImVec2 node_half_size = size * 0.5f;
    ImVec2 center_offset_half = (data.bb.Max - data.bb.Min) * .5f;
    ImVec2 node_frame_min = (data.bb.Min + center_offset_half + (*pos - node_half_size + data.offset_plus_drag_delta) * data.zoom);
    ImVec2 node_frame_max = node_frame_min + size * data.zoom;
    s_node_bb = ImRect(node_frame_min, node_frame_max);
    s_node_next_in_pos = s_node_bb.Min + ImVec2(0, ImGui::GetTextLineHeight() * 3.0f);
    s_node_next_out_pos = ImVec2(s_node_bb.Max.x, s_node_bb.Min.y) + ImVec2(0, ImGui::GetTextLineHeight() * 3.0f);
    
    s_node_selected = selected;
    s_node_clicked_ptr = clicked;

    s_node_col = col;

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
    auto& data = *cur_grid_data;

    bool clicked = false;

    ImU32 node_col = s_node_col;//ImGui::GetColorU32(ImGuiCol_WindowBg, 1);

    if((s_node_drag_id == ImGui::GetID(s_node_name.c_str())) && ImGui::IsMouseDragging(0)) {
        ImVec2 mdd = ImGui::GetMouseDragDelta(0);
        *s_node_pos_out += mdd / data.zoom;
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

    ImVec2 shadow_ofs = ImVec2(10, 10) * data.zoom;
    ImGui::RenderFrame(
        s_node_bb.Min + shadow_ofs, s_node_bb.Max + shadow_ofs, 
        ImGui::GetColorU32(ImGuiCol_WindowBg, 0.5f), true, 10.0f * data.zoom
    );

    ImGui::RenderFrame(
        s_node_bb.Min, s_node_bb.Max, 
        ImGui::GetColorU32(ImGuiCol_WindowBg, 1), true, 10.0f * data.zoom //ImGui::GetStyle().FrameRounding
    );
    ImGui::RenderFrame(
        s_node_bb.Min, ImVec2(s_node_bb.Max.x, s_node_bb.Min.y + ImGui::GetTextLineHeight() * 2), 
        node_col, true, 10.0f * data.zoom //ImGui::GetStyle().FrameRounding
    );
    if(
        (s_node_selected || s_node_drag_id == ImGui::GetID(s_node_name.c_str()) || ImGui::IsMouseHoveringRect(s_node_bb.Min, s_node_bb.Max, true))
    ) {
        ImGui::GetWindowDrawList()->AddRect(
            s_node_bb.Min, s_node_bb.Max, ImGui::GetColorU32(ImGuiCol_Text, 1.0f),
            10.0f * data.zoom, 15, 2.0f
        );
    }
    ImGui::RenderText(ImGui::GetStyle().WindowPadding * data.zoom + s_node_bb.Min, s_node_name.c_str());

    ImGui::GetWindowDrawList()->ChannelsMerge();

    s_node_selected = false;
    s_node_clicked_ptr = 0;
}

int64_t TreeNodeIn(const char* name, uint64_t user_id, uint64_t other_user_id, ImU32 connection_col) {
    auto& data = *cur_grid_data;

    bool new_connection_occured = false;
    ImGui::GetWindowDrawList()->ChannelsSetCurrent(1);

    if(ImGui::IsMouseHoveringRect(s_node_next_in_pos + ImVec2(-10, -10) * data.zoom, s_node_next_in_pos + ImVec2(10, 10) * data.zoom, true)) {
        if(ImGui::IsMouseClicked(0)) {
            if(s_new_conn_type == NEW_CONN_NONE || s_new_conn_type == NEW_CONN_IN) {
                s_new_conn_grid_owner = data.id;
                s_new_conn_type = NEW_CONN_IN;
                s_new_connection_origin = GridScreenToPos(s_node_next_in_pos, data);
                s_potential_connection_origin_id = user_id;
            } else if (s_new_conn_type == NEW_CONN_OUT && s_new_conn_grid_owner == data.id) {
                s_new_conn_type = NEW_CONN_NONE;
                new_connection_occured = true;
                other_user_id = s_potential_connection_origin_id;
            }
        }
        ImGui::GetWindowDrawList()->AddCircleFilled(
            s_node_next_in_pos, 9.5f * data.zoom, ImGui::GetColorU32(ImGuiCol_Text), 12
        );
    } else {
        ImGui::GetWindowDrawList()->AddCircle(
            s_node_next_in_pos, 8 * data.zoom, ImGui::GetColorU32(ImGuiCol_Text), 12, 3.0f * data.zoom
        );
    }
    ImGui::GetWindowDrawList()->AddText(s_node_next_in_pos + ImVec2(20, 0) * data.zoom - ImVec2(0, ImGui::GetTextLineHeight()) * 0.5f, ImGui::GetColorU32(ImGuiCol_Text), name);

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
        s_connections.insert(Connection{ other_user_id, user_id, connection_col });
    }
    
    if(new_connection_occured) {
        return other_user_id;
    } else {
        return 0;
    }
}

int64_t TreeNodeOut(const char* name, uint64_t user_id, uint64_t other_user_id) {
    auto& data = *cur_grid_data;

    bool new_connection_occured = false;
    ImGui::GetWindowDrawList()->ChannelsSetCurrent(1);
    
    if(ImGui::IsMouseHoveringRect(s_node_next_out_pos + ImVec2(-10, -10) * data.zoom, s_node_next_out_pos + ImVec2(10, 10) * data.zoom, true)) {
        if(ImGui::IsMouseClicked(0)) {
            if(s_new_conn_type == NEW_CONN_NONE || s_new_conn_type == NEW_CONN_OUT) {
                s_new_conn_grid_owner = data.id;
                s_new_conn_type = NEW_CONN_OUT;
                s_new_connection_origin = GridScreenToPos(s_node_next_out_pos, data);
                s_potential_connection_origin_id = user_id;
            } else if(s_new_conn_type == NEW_CONN_IN && s_new_conn_grid_owner == data.id) {
                s_new_conn_type = NEW_CONN_NONE;
                new_connection_occured = true;
                other_user_id = s_potential_connection_origin_id;
            }
        }
        ImGui::GetWindowDrawList()->AddCircleFilled(
            s_node_next_out_pos, 9.5f * data.zoom, ImGui::GetColorU32(ImGuiCol_Text), 12
        );
    } else {
        ImGui::GetWindowDrawList()->AddCircle(
            s_node_next_out_pos, 8 * data.zoom, ImGui::GetColorU32(ImGuiCol_Text), 12, 3.0f * data.zoom
        );
    }
    ImVec2 text_size = ImGui::CalcTextSize(name);
    ImGui::GetWindowDrawList()->AddText(s_node_next_out_pos - ImVec2(20, 0) * data.zoom - ImVec2(text_size.x, 0) - ImVec2(0, ImGui::GetTextLineHeight()) * 0.5f, ImGui::GetColorU32(ImGuiCol_Text), name);

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
        s_connections.insert(Connection{ user_id, other_user_id, ImGui::GetColorU32(ImGuiCol_Text) });
    }

    if(new_connection_occured) {
        return other_user_id;
    } else {
        return 0;
    }
}

void TreeNodeMarkedConnection(uint64_t a_, uint64_t b_, float w) {
    auto& data = *cur_grid_data;
    ImGui::GetWindowDrawList()->ChannelsSetCurrent(1);
    ImVec2 p1 = s_node_point_coords[a_];
    ImVec2 p4 = s_node_point_coords[b_];
    ImVec2 p2 = p1 + ImVec2(100, 0) * data.zoom;
    ImVec2 p3 = p4 - ImVec2(100, 0) * data.zoom;
    
    const int num_segments = 12;
    gfxm::vec2 points[13];
    points[0] = gfxm::vec2(p1.x, p1.y);
    float t_step = 1.0f / (float)num_segments;
    for (int i_step = 1; i_step <= num_segments; i_step++)
    {
        float t = t_step * i_step;
        float u = 1.0f - t;
        float w1 = u*u*u;
        float w2 = 3*u*u*t;
        float w3 = 3*u*t*t;
        float w4 = t*t*t;
        points[i_step] = gfxm::vec2(w1*p1.x + w2*p2.x + w3*p3.x + w4*p4.x, w1*p1.y + w2*p2.y + w3*p3.y + w4*p4.y);
    }

    static float base_f = .0f;
    base_f += 1.0f/60.0f * 0.1f;
    if(base_f > 1.0f) {
        base_f = .0f;
    }
    for(float f = base_f / 12.0f; f <= 1.0f; f += 1.0f / 6.0f) {
        gfxm::vec2 point;
        int id = (int)(12 * f);
        int id2 = id + 1;
        if(id2 > 12) {
            point = points[12];
        } else {
            gfxm::vec2 a = points[id];
            gfxm::vec2 b = points[id2];
            float w      = (12 * f) - (float)(id);
            point = gfxm::lerp(a, b, w);
            ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(point.x, point.y), 6, ImGui::GetColorU32(ImGuiCol_Text));
        }
    }
}

void CommitTreeNodeConnections() {
    auto& data = *cur_grid_data;

    for(auto& it : s_connections) {
        ImVec2 a = s_node_point_coords[(it).a];
        ImVec2 b = s_node_point_coords[(it).b];

        ImGui::GetWindowDrawList()->AddBezierCurve(
            a,
            a + ImVec2(100, 0) * data.zoom, b + ImVec2(-100, 0) * data.zoom,
            b,
            (it).color,
            3.0f * data.zoom, 50
        );
    }
}

void TreeNodeConnection(size_t node_from, size_t node_to, size_t out_n, size_t in_n) {
    auto& data = *cur_grid_data;

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
        a + ImVec2(100, 0) * data.zoom, b + ImVec2(-100, 0) * data.zoom,
        b,
        ImGui::GetColorU32(ImGuiCol_Text),
        3.0f * data.zoom, 50
    );
}

void NodeConnection(const ImVec2& from, const ImVec2& to) {
    auto& data = *cur_grid_data;

    ImVec2 ref_pos = (data.bb.Min + (data.offset_plus_drag_delta) * data.zoom);

    ImVec2 p0 = ref_pos + from * data.zoom;
    ImVec2 cp0 = ref_pos + (from + ImVec2(100,0)) * data.zoom;
    ImVec2 p1 = ref_pos + to * data.zoom;
    ImVec2 cp1 = ref_pos + (to + ImVec2(-100, 0)) * data.zoom;

    ImGui::GetWindowDrawList()->AddBezierCurve(
        p0,
        cp0, cp1,
        p1,
        ImGui::GetColorU32(ImGuiCol_Text),
        3.0f * data.zoom, 50
    );
}

bool BeginGridView(const char* id, const ImVec2& sz) {
    cur_grid_data = &s_grid_data[id];
    auto& data = *cur_grid_data;
    if(ImGui::BeginChild(id, sz)) {
        data.id = ImGui::GetID(id);
        std::string name = id;
        auto window = ImGui::GetCurrentWindow();

        auto bb = window->ContentsRegionRect;
        bb = window->WorkRect;
        bb = window->InnerRect;
        if(sz.x > 0) {
            bb.Max.x = bb.Min.x + sz.x;
        }
        if(sz.y > 0) {
            bb.Max.y = bb.Min.y + sz.y;
        }
        data.bb = bb;
        ImVec2 size = ImVec2(bb.Max.x - bb.Min.x, bb.Max.y - bb.Min.y);

        ImGuiIO& io = ImGui::GetIO();
        ImVec2 cursor_pos = io.MousePos;
        cursor_pos.x = cursor_pos.x - bb.Min.x;
        cursor_pos.y = cursor_pos.y - bb.Min.y;

        bool hovered = ImGui::IsWindowHovered();

        if(io.MouseWheel != .0f && hovered) {
            ImVec2 observed_size_before_zoom_change = size / data.zoom;
            data.zoom += io.MouseWheel * 0.1f;
            if(data.zoom < 0.1f) {
                data.zoom = 0.1f;
            }
            ImVec2 observed_size_after_zoom_change = size / data.zoom;
            ImVec2 observed_box_size_delta = observed_size_before_zoom_change - observed_size_after_zoom_change;
            ImVec2 cursor_pos_factor = (cursor_pos - size * .5f) / size;
            ImVec2 zoom_pos_adjustment = observed_box_size_delta * cursor_pos_factor;
            data.offset -= zoom_pos_adjustment;
        }

        if(hovered && ImGui::IsMouseDown(2)) {
            data.scroll_id = data.id;
        }
        
        if(data.scroll_id == data.id) {
            data.tmp_offset = ImGui::GetMouseDragDelta(2) / data.zoom;
        }

        data.offset_plus_drag_delta = data.offset + data.tmp_offset;
        if(data.scroll_id == data.id && ImGui::IsMouseReleased(2)) {
            data.offset += data.tmp_offset;
            data.scroll_id = 0;
        }
        data.tmp_offset = ImVec2();

        ImGui::PushClipRect(bb.Min, bb.Max, false);
        ImGui::RenderFrame(
            bb.Min, bb.Max, 
            ImGui::GetColorU32(ImGuiCol_FrameBg, 1), true, ImGui::GetStyle().FrameRounding
        );

        float grid_step = 100.0f * data.zoom;
        ImVec2 half_size = size * .5f / data.zoom;
        float ax = (data.offset_plus_drag_delta.x + half_size.x) / 100.0f;
        ax = ax - (int)ax;
        float ay = (data.offset_plus_drag_delta.y + half_size.y) / 100.0f;
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

        ImGui::SetWindowFontScale(data.zoom);
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
    auto& data = *cur_grid_data;
    CommitTreeNodeConnections();

    if(s_new_conn_type != NEW_CONN_NONE && s_new_conn_grid_owner == data.id) {
        ImVec2 a;
        ImVec2 b;
        if(s_new_conn_type == NEW_CONN_OUT) {
            a = s_new_connection_origin;
            b = GridScreenToPos(ImGui::GetMousePos(), data);
        } else if(s_new_conn_type == NEW_CONN_IN) {
            a = GridScreenToPos(ImGui::GetMousePos(), data);
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

    ImGui::RenderText(data.bb.Min, MKSTR("Zoom: " << data.zoom).c_str());
    ImGui::RenderText(data.bb.Min + ImVec2(0, ImGui::GetTextLineHeight() * 1), MKSTR("x: " << data.offset_plus_drag_delta.x).c_str());
    ImGui::RenderText(data.bb.Min + ImVec2(0, ImGui::GetTextLineHeight() * 2), MKSTR("y: " << data.offset_plus_drag_delta.y).c_str());
}


struct BlendspaceData {
    ImRect frame_bb;
    ImRect normal_bb;
};
static std::unordered_map<ImGuiID, BlendspaceData> blendspaces;
static BlendspaceData* cur_blendspace = 0;
bool BeginBlendspace(const char* label, ImVec2& frame_size, const ImVec2& grid_spacing) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    auto& data = blendspaces[window->GetID(label)];
    cur_blendspace = &data;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
    if (frame_size.x == 0.0f) {
        frame_size.x = ImGui::CalcItemWidth();
    }
    if (frame_size.y == 0.0f) {
        //frame_size.y = label_size.y + (style.FramePadding.y * 2);
        frame_size.y = label_size.y + frame_size.x;
    }

    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + frame_size);
    const ImRect inner_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);
    const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0));
    const ImRect normal_bb(frame_bb.Min + (frame_bb.Max - frame_bb.Min) * 0.10f, frame_bb.Max - (frame_bb.Max - frame_bb.Min) * 0.10f);
    const ImVec2 normal_size(normal_bb.Max.x - normal_bb.Min.x, normal_bb.Max.y - normal_bb.Min.y);
    data.frame_bb = frame_bb;
    data.normal_bb = normal_bb;
    ImGui::ItemSize(total_bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(total_bb, 0, &frame_bb))
        return false;
    const bool hovered = ImGui::ItemHoverable(frame_bb, id);

    ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);
    ImU32 col_grid = ImGui::GetColorU32(ImGuiCol_Border, 1.0f);
    for(float x = 0; x <= normal_size.x; x += normal_size.x / grid_spacing.x) {
        ImVec2 t(normal_bb.Min.x + x, normal_bb.Min.y);
        ImVec2 b(normal_bb.Min.x + x, normal_bb.Max.y);
        ImGui::GetWindowDrawList()->AddLine(t, b, col_grid);
    }
    for(float y = 0; y <= normal_size.y; y += normal_size.y / grid_spacing.y) {
        ImVec2 l(normal_bb.Min.x, normal_bb.Min.y + y);
        ImVec2 r(normal_bb.Max.x, normal_bb.Min.y + y);
        ImGui::GetWindowDrawList()->AddLine(l, r, col_grid);
    }
    


    if (label_size.x > 0.0f) {
        ImGui::RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, inner_bb.Min.y), label);
    }
    ImGui::PushClipRect(frame_bb.Min, frame_bb.Max, true);
    return true;
}
void EndBlendspace() {
    cur_blendspace = 0;
    ImGui::PopClipRect();
}
bool BlendspacePoint(const char* name, ImVec2& pos, float radius, const ImVec2& snap, ImU32 col) {
    bool moved = false;
    assert(cur_blendspace);
    auto& data = *cur_blendspace;
    ImGuiIO& io = ImGui::GetIO();
    ImRect frame_bb = data.frame_bb;
    ImRect n_bb     = data.normal_bb;
    ImVec2 frame_size(frame_bb.Max.x - frame_bb.Min.x, frame_bb.Max.y - frame_bb.Min.y);
    ImVec2 normal_size(n_bb.Max.x - n_bb.Min.x, n_bb.Max.y - n_bb.Min.y);
    ImVec2 p(
        n_bb.Min.x + (n_bb.Max.x - n_bb.Min.x) * pos.x,
        n_bb.Min.y + (n_bb.Max.y - n_bb.Min.y) * pos.y
    );
    bool hovering = false;
    if(ImGui::IsMouseHoveringRect(p - ImVec2(radius, radius), p + ImVec2(radius, radius))) {
        hovering = true;
        if(ImGui::IsMouseDown(0) && !ImGui::IsMouseDragging(0)) {
            s_node_drag_id = ImGui::GetID(name);
            moved = true;
        }
    }
    if(ImGui::IsMouseReleased(0)) {
        s_node_drag_id = 0;
    }
    if((s_node_drag_id == ImGui::GetID(name)) && ImGui::IsMouseDragging(0)) {
        ImVec2 mdd = ImGui::GetMouseDragDelta(0);
        pos = (io.MousePos - n_bb.Min) / normal_size;
        if (snap.x != .0f) {
            pos.x = pos.x * snap.x;
            pos.x = round(pos.x);
            pos.x = pos.x / snap.x;
        }
        if(snap.y != .0f) {
            pos.y = pos.y * snap.y;
            pos.y = round(pos.y);
            pos.y = pos.y / snap.y;
        }
        pos.x = gfxm::_max(.0f, gfxm::_min(1.0f, pos.x));
        pos.y = gfxm::_max(.0f, gfxm::_min(1.0f, pos.y));
        ImGui::ResetMouseDragDelta(0);
        moved = true;
    }
    if(hovering || moved) {
        ImGui::GetWindowDrawList()->AddCircle(
            p, radius, col, 12, 2.0f
        );
    } else {
        ImGui::GetWindowDrawList()->AddCircleFilled(
            p, radius, col, 12
        );
    }
    ImGui::GetWindowDrawList()->AddText(
        p + ImVec2(10, 10) - ImVec2(0, ImGui::GetTextLineHeight()) * 0.5f, 
        ImGui::GetColorU32(ImGuiCol_Text), name
    );
    return moved;
}
void BlendspaceLine(const ImVec2& a, const ImVec2& b, ImU32 col) {
    assert(cur_blendspace);
    auto& data = *cur_blendspace;
    ImGuiIO& io = ImGui::GetIO();
    ImRect frame_bb = data.frame_bb;
    ImRect n_bb     = data.normal_bb;
    ImVec2 frame_size(frame_bb.Max.x - frame_bb.Min.x, frame_bb.Max.y - frame_bb.Min.y);
    ImVec2 normal_size(n_bb.Max.x - n_bb.Min.x, n_bb.Max.y - n_bb.Min.y);
    ImVec2 f(
        n_bb.Min.x + (n_bb.Max.x - n_bb.Min.x) * a.x,
        n_bb.Min.y + (n_bb.Max.y - n_bb.Min.y) * a.y
    );
    ImVec2 t(
        n_bb.Min.x + (n_bb.Max.x - n_bb.Min.x) * b.x,
        n_bb.Min.y + (n_bb.Max.y - n_bb.Min.y) * b.y
    );
    ImGui::GetWindowDrawList()->AddLine(f, t, col, 1.0f);
}


static ImVec2 inset_segment_min;
static ImU32 inset_segment_col;
static float inset_segment_orig_cursor_x;
static float inset_segment_orig_region_width;
static float inset_segment_line_height;
void BeginInsetSegment(ImU32 bg_color) {
    inset_segment_orig_cursor_x = ImGui::GetCursorPosX();
    inset_segment_line_height = ImGui::GetTextLineHeight();

    inset_segment_col = bg_color;
    ImGui::GetWindowDrawList()->ChannelsSplit(2);
    ImGui::GetWindowDrawList()->ChannelsSetCurrent(1);
    inset_segment_min = ImGui::GetCursorPos() + ImGui::GetWindowPos();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().WindowPadding.y);

    ImGui::Indent(ImGui::GetStyle().WindowPadding.x);
    //ImGui::PushItemWidth(-ImGui::GetStyle().WindowPadding.x);
    //ImGui::SetCursorPosX(ImGui::GetStyle().WindowPadding.x + inset_segment_orig_cursor_x);
}
void EndInsetSegment() {
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().WindowPadding.y);
    
    ImVec2 inset_segment_max = ImGui::GetCursorPos() + ImGui::GetWindowPos();
    inset_segment_max.x = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionWidth();
    ImGui::GetWindowDrawList()->ChannelsSetCurrent(0);
    ImGui::RenderFrame(inset_segment_min, inset_segment_max, inset_segment_col, false, 10.0f);
    ImGui::GetWindowDrawList()->ChannelsMerge();

    ImGui::Unindent(ImGui::GetStyle().WindowPadding.x);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().WindowPadding.y);
    //ImGui::PopItemWidth();
    //ImGui::SetCursorPosX(inset_segment_orig_cursor_x);
}


}