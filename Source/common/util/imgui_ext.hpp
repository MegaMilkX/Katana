#ifndef IMGUI_EXT_HPP
#define IMGUI_EXT_HPP

#include <stdint.h>

#include "../lib/imgui/imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../lib/imgui/imgui_internal.h"

namespace ImGuiExt {


void  BeginTreeNode(const char* name, ImVec2* pos, bool* clicked, bool selected, const ImVec2& size, ImU32 col);
void  EndTreeNode();
int64_t TreeNodeIn(const char* name, int64_t user_id, int64_t other_user_id);
int64_t TreeNodeOut(const char* name, int64_t user_id, int64_t other_user_id);

void  TreeNodeConnection(size_t node_from, size_t node_to, size_t out_n, size_t in_n);

void  NodeConnection(const ImVec2& from, const ImVec2& to);

bool  BeginGridView(const char* name);
void  EndGridView();


bool BeginTimeline(float length, float* cursor, const ImVec2& sz = ImVec2(0,0));
void EndTimeline();

void TimelineEvent(const char* label, float pos);
void TimelineMarker(const char* label, float pos);


void BeginInsetSegment(ImU32 bg_color = ImGui::GetColorU32(ImGuiCol_FrameBg));
void EndInsetSegment();


}

#endif
