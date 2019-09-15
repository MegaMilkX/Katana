#include "imgui_ext.hpp"

#include <string>

namespace ImGuiExt {

static bool s_dragging = false;
static float s_drag_start_x = .0f;
static ImRect s_timeline_bb;
static float s_timeline_length = .0f;
static ImVec2 s_timeline_size;

bool BeginTimeline(float length, float* cursor, const ImVec2& sz) {
    auto window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    auto bb = window->ContentsRegionRect;
    bb.Max.y = bb.Min.y + ImGui::GetTextLineHeightWithSpacing();
    ImVec2 size(bb.Max.x - bb.Min.x, bb.Max.y - bb.Min.y);
    bb = ImRect(window->DC.CursorPos, window->DC.CursorPos + size);
    s_timeline_bb = bb;
    s_timeline_size = size;
    s_timeline_length = length;

    float cursor_pos_norm = .0f;

    if(ImGui::ItemAdd(bb, ImGui::GetID("TIMELINE"))) {
        ImGui::RenderFrame(
            bb.Min, bb.Max, 
            ImGui::GetColorU32(ImGuiCol_FrameBg, 1), true, ImGui::GetStyle().FrameRounding
        );

        if(ImGui::IsMouseClicked(0) && ImGui::IsItemHovered()) {
            s_drag_start_x = ImGui::GetMousePos().x;
            s_dragging = true;
        }
        if(ImGui::IsMouseReleased(0)) {
            s_dragging = false;
        }

        ;
        if(s_dragging) {
            cursor_pos_norm = ((s_drag_start_x + ImGui::GetMouseDragDelta(0).x) - bb.Min.x) / size.x;
            if(cursor) {
                *cursor = length * cursor_pos_norm;
            }
        }
        if(cursor) {
            *cursor = std::round(*cursor);
            cursor_pos_norm = *cursor / length;
        }
        float cursor_pos = size.x * cursor_pos_norm;
        cursor_pos += bb.Min.x;

        int step = length / 5 + 0.1f;

        for(int i = 0; i <= length; ++i) {
            ImVec2 a(bb.Min.x + (i / length) * size.x, bb.Min.y);
            ImVec2 b(bb.Min.x + (i / length) * size.x, bb.Max.y);
            ImGui::GetWindowDrawList()->AddLine(
                a, b, ImGui::GetColorU32(ImGuiCol_TextDisabled)
            );

            if(i % step == 0) { 
                ImGui::RenderText(a, std::to_string(i).c_str());
            }
        }

        ImVec2 a(cursor_pos - 5, bb.Min.y);
        ImVec2 b(cursor_pos, bb.Min.y + 5);
        ImVec2 c(cursor_pos + 5, bb.Min.y);
        ImGui::GetWindowDrawList()->AddTriangleFilled(a, b, c, ImGui::GetColorU32(ImGuiCol_Text));

        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(cursor_pos, bb.Min.y),
            ImVec2(cursor_pos, bb.Max.y),
            ImGui::GetColorU32(ImGuiCol_Text),
            2.0f
        );
    }
    
    return true;
}

void EndTimeline() {

}

void TimelineEvent(const char* label, float pos) {
    float pos_norm = pos / s_timeline_length;
    float screen_pos = s_timeline_size.x * pos_norm;
    screen_pos += s_timeline_bb.Min.x;

    const float marker_radius = 5;

    ImVec2 marker_pos(screen_pos, s_timeline_bb.Min.y);

    ImVec2 text_sz = ImGui::CalcTextSize(label);
    ImVec2 text_pos = marker_pos;
    text_pos.x -= text_sz.x * 0.5f;
    text_pos.y -= text_sz.y + marker_radius;

    ImU32 col = ImGui::GetColorU32(ImGuiCol_PlotHistogram);

    ImGui::GetWindowDrawList()->AddCircleFilled(
        marker_pos,
        marker_radius,
        col,
        4
    );

    ImGui::GetWindowDrawList()->AddText(text_pos, col, label);
}

void TimelineMarker(const char* label, float pos) {
    float pos_norm = pos / s_timeline_length;
    float screen_pos = s_timeline_size.x * pos_norm;
    screen_pos += s_timeline_bb.Min.x;

    const float marker_radius = 5;
    ImVec2 marker_pos(screen_pos, s_timeline_bb.Min.y);

    ImVec2 text_sz = ImGui::CalcTextSize(label);
    ImVec2 text_pos(screen_pos, s_timeline_bb.Min.y);
    text_pos.x -= text_sz.x * 0.5f;
    text_pos.y -= text_sz.y + marker_radius;

    ImU32 col = ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 1.0f, 0.4f, 1.0f));

    ImGui::GetWindowDrawList()->AddQuadFilled(
        marker_pos + ImVec2(-marker_radius, -marker_radius),
        marker_pos + ImVec2(marker_radius, -marker_radius),
        marker_pos + ImVec2(marker_radius, marker_radius),
        marker_pos + ImVec2(-marker_radius, marker_radius),
        col
    );

    ImGui::GetWindowDrawList()->AddText(text_pos, col, label);
}

}