#ifndef IMGUI_WRAP_H
#define IMGUI_WRAP_H

#include "imgui/imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui_internal.h"

#include "../gfxm.hpp"
#include "../util/log.hpp"

void ImGuiInit();
void ImGuiCleanup();
void ImGuiUpdate(float dt, int w, int h);
void ImGuiDraw();

namespace ImGui {

static ImVec2 timeline_size;
static ImRect timeline_bb;
static ImGuiWindow* timeline_window;
static float timeline_length = 1.0f;

inline bool BeginTimeline(const char* label, float length = 1.0f) {
    timeline_window = ImGui::GetCurrentWindow();
    auto window = timeline_window;
    timeline_size = ImVec2(window->Size.x, 40);
    auto& size = timeline_size;
    timeline_length = length;

    ImGui::Text(label);
    if(!ImGui::BeginChild(label, timeline_size)) {
        return false;
    }
    timeline_bb = ImRect(window->DC.CursorPos, window->DC.CursorPos + size);
    auto& bb = timeline_bb;
    bb = ImGui::GetCurrentWindow()->ContentsRegionRect;
    bb.Max.x -= 30;
    bb.Min.x += 10;
    size = ImVec2(bb.Max.x - bb.Min.x, bb.Max.y - bb.Min.y);

    window->DrawList->AddLine(
        ImVec2(bb.Min.x, bb.Min.y + size.y / 2),
        ImVec2(bb.Max.x, bb.Min.y + size.y / 2),
        ImGui::GetColorU32(ImGuiCol_TextDisabled), 3
    );

    return true;
} 

static bool marker_dragging = false;
static std::string marker_drag_label;
static float marker_drag_pos_tmp = .0f;

inline bool TimelineMarker(const char* label, float& out_pos, bool* selected = 0) {
    auto& bb = timeline_bb;
    auto& size = timeline_size;
    auto window = timeline_window;

    bool was_selected = false;

    if(ImGui::IsMouseReleased(0)) {
        marker_dragging = false;
        if(marker_drag_label == label) {
            out_pos = marker_drag_pos_tmp;
            was_selected = true;
            marker_drag_label = "";
        }
    }
    float pos = out_pos;

    if(marker_dragging && marker_drag_label == label) {
        ImVec2 mdelta = ImGui::GetMouseDragDelta(0);
        
        pos += mdelta.x * (timeline_length / (bb.Max.x - bb.Min.x));
        if(pos < .0f) pos = .0f;
        if(pos > timeline_length) pos = timeline_length;
        marker_drag_pos_tmp = pos;
    }
    
    ImVec2 p(pos / timeline_length, 0.0f);
    p.y = bb.Min.y + size.y / 2;
    //p = p * (timeline_bb.Max - timeline_bb.Min) + timeline_bb.Min;
    p.x = bb.Min.x + p.x * (bb.Max.x - bb.Min.x);
    ImVec2 a = p - ImVec2(4, 4);
    ImVec2 b = p + ImVec2(4, 4);

    ImU32 marker_color = ImGui::GetColorU32(ImGuiCol_TextDisabled);
    if(selected && *selected) {
        marker_color = ImGui::GetColorU32(ImGuiCol_Text);
    }
    if(ImGui::IsWindowHovered()) {
        if(ImGui::IsMouseHoveringRect(a, b)) {
            marker_color = ImGui::GetColorU32(ImGuiCol_PlotHistogram);
            if(ImGui::IsMouseClicked(0)) {
                marker_dragging = true;
                marker_drag_label = label;
                was_selected = true;
            }       
        }
    }

    window->DrawList->AddRectFilled(a, b, marker_color);
    window->DrawList->AddText(ImVec2(a.x, b.y), marker_color, label);
    window->DrawList->AddText(ImVec2(a.x, a.y - 15), marker_color, MKSTR(pos).c_str());
    return was_selected;
}

inline void EndTimeline() {
    ImGui::EndChild();
}

}

namespace ImGui {

static int curve_edit_id = 0;
static int curve_drag_id = -1;
static ImVec2 curve_mpos_prev;
static std::string curve_drag_name;

inline bool CurveEdit(const char* id, ImVec2* points, size_t& point_count, size_t max_points, float length = 1.0f, float y_min = -1.0f, float y_max = 1.0f, int smoothness = 256) {
    bool changed = false;
    float height = y_max - y_min;
    std::string name = id;
    auto window = ImGui::GetCurrentWindow();
    ImVec2 size = ImVec2(window->Size.x, 40);

    ++curve_edit_id;
    if(!ImGui::BeginChild(name.c_str(), size + ImVec2(0, 15))) {
        return false;
    }
    window = ImGui::GetCurrentWindow();
    auto bb = window->ContentsRegionRect;
    bb.Max.x -= 20;
    bb.Min.x += 0;
    bb.Max.y -= 15;
    size = ImVec2(bb.Max.x - bb.Min.x, bb.Max.y - bb.Min.y);

    ImGui::RenderFrame(
        bb.Min, bb.Max, 
        ImGui::GetColorU32(ImGuiCol_FrameBg, 1), true, ImGui::GetStyle().FrameRounding
    );

    ImU32 text_color = ImGui::GetColorU32(ImGuiCol_TextDisabled);

    for(float v = y_min; v <= y_max + 0.5f; v += 1.0f) {
        float y = (bb.Max.y - size.y * 0.5f) - size.y * 0.5f * v;
        window->DrawList->AddLine(
            ImVec2(bb.Min.x, y),
            ImVec2(bb.Max.x, y),
            text_color
        );
        window->DrawList->AddText(ImVec2(bb.Min.x, y), text_color, MKSTR(v).c_str());
    }
    for(float v = .0f; v <= length + 0.5f; v += 0.5f) {
        float x = (bb.Max.x - size.x) + size.x * v;
        window->DrawList->AddLine(
            ImVec2(x, bb.Min.y),
            ImVec2(x, bb.Max.y),
            text_color
        );
    }

    if(curve_drag_id >= 0 && curve_drag_name == name) {
        ImVec2 mdrag = ImGui::GetMousePos() - curve_mpos_prev;
        mdrag.x *= length / size.x;
        mdrag.y *= -(y_max - y_min) / size.y;
        points[curve_drag_id] += mdrag;
        auto& pt = points[curve_drag_id];
        if(pt.x < 0.0f) pt.x = 0.0f;
        else if(pt.x > length) pt.x = length;
        if(pt.y < y_min) pt.y = y_min;
        else if(pt.y > y_max) pt.y = y_max;
        if(curve_drag_id > 0) {
            if(pt.x < points[curve_drag_id - 1].x) pt.x = points[curve_drag_id - 1].x;
        }
        if(curve_drag_id < point_count - 1) {
            if(pt.x > points[curve_drag_id + 1].x) pt.x = points[curve_drag_id + 1].x;
        }
        changed = true;
        curve_mpos_prev = ImGui::GetMousePos();
    }
    if(ImGui::IsWindowHovered()) {        
        if(ImGui::IsMouseDoubleClicked(0)) {
            ImVec2 mpos = ImGui::GetMousePos();
            ImVec2 frame_space_pos(
                (mpos.x - bb.Min.x) / size.x * length,
                height * 0.5f - ((mpos.y - bb.Min.y) / size.y) * height
            );
            if(point_count + 1 <= max_points) {
                size_t left_neighbor = -1;
                size_t new_point_index = 0;
                for(size_t i = 0; i < point_count; ++i) {
                    if(points[i].x < frame_space_pos.x) {
                        new_point_index = i + 1;
                        left_neighbor = i;
                    }
                    else 
                        break;
                }
                for(size_t i = point_count; i > new_point_index; --i) {
                    points[i] = points[i - 1];
                }
                ImVec2& p = points[new_point_index];
                p = frame_space_pos;
                point_count++;
                changed = true;
            }
        }
    }
    if(ImGui::IsMouseReleased(0)) {
        curve_drag_id = -1;
    }

    if(point_count < 1) {
        ImGui::EndChild();
        return changed;
    }

    // Curve
    if(point_count > 0) {
        ImVec2 p0(points[0].x, points[0].y);
        p0.y = bb.Min.y + size.y / height - (p0.y / height * size.y);
        p0.x = bb.Min.x + (p0.x / length) * size.x;
        ImVec2 p1(points[point_count - 1].x, points[point_count - 1].y);
        p1.y = bb.Min.y + size.y / height - (p1.y / height * size.y);
        p1.x = bb.Min.x + (p1.x / length) * size.x;
        window->DrawList->AddLine(
            ImVec2(bb.Min.x, p0.y), 
            ImVec2(p0.x, p0.y), 
            ImGui::GetColorU32(ImGuiCol_PlotLinesHovered, 1)
        );
        window->DrawList->AddLine(
            ImVec2(p1.x, p1.y), 
            ImVec2(bb.Max.x, p1.y), 
            ImGui::GetColorU32(ImGuiCol_PlotLinesHovered, 1)
        );
    }
    for(size_t i = 0; i < point_count - 1; ++i) {
        ImVec2 p0(points[i].x, points[i].y);
        p0.y = bb.Min.y + size.y / height - (p0.y / height * size.y);
        p0.x = bb.Min.x + (p0.x / length) * size.x;
        ImVec2 p1(points[i+1].x, points[i+1].y);
        p1.y = bb.Min.y + size.y / height - (p1.y / height * size.y);
        p1.x = bb.Min.x + (p1.x / length) * size.x;

        float node_distance = (p1.x - p0.x);
        float line_step = (node_distance / size.x);
        for(float f = p0.x; f < p1.x; f += line_step) {
            float y0 = gfxm::smoothstep(
                p0.y, p1.y, 
                (f - p0.x) / node_distance
            );
            float y1 = gfxm::smoothstep(
                p0.y, p1.y, 
                (f + line_step - p0.x) / node_distance
            );
            window->DrawList->AddLine(
                ImVec2(f, y0), 
                ImVec2(f + line_step, y1), 
                ImGui::GetColorU32(ImGuiCol_PlotLinesHovered, 1)
            );
        }
    }

    // Control points
    for(size_t i = 0; i < point_count; ++i) {
        ImU32 marker_color = ImGui::GetColorU32(ImGuiCol_TextDisabled);

        ImVec2 p(points[i].x, points[i].y);
        p.y = bb.Min.y + size.y / height - (p.y / height * size.y);
        p.x = bb.Min.x + (p.x / length) * size.x;
        ImVec2 a = p - ImVec2(4, 4);
        ImVec2 b = p + ImVec2(4, 4);
        if(ImGui::IsWindowHovered()) {
            if(ImGui::IsMouseHoveringRect(a, b)) {
                marker_color = ImGui::GetColorU32(ImGuiCol_PlotHistogram);
                window->DrawList->AddText(a + ImVec2(0, -15), marker_color, MKSTR("[" << points[i].x << ", " << points[i].y << "]").c_str());
                if(ImGui::IsMouseClicked(0)) {
                    curve_drag_id = i;
                    curve_drag_name = name;
                    curve_mpos_prev = ImGui::GetMousePos();
                }
                if(ImGui::IsMouseClicked(1)) {
                    if(point_count > 0) {
                        for(size_t j = i; j < point_count - 1; ++j) {
                            points[j] = points[j + 1];
                        }
                        point_count--;
                        changed = true;
                    }
                }
            }
        }
        window->DrawList->AddRectFilled(a, b, marker_color, 1);
    }

    ImGui::EndChild();
    return changed;
}

}

#endif
