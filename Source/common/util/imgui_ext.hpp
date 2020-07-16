#ifndef IMGUI_EXT_HPP
#define IMGUI_EXT_HPP

#include <stdint.h>
#include <string>

#include "../lib/imgui/imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../lib/imgui/imgui_internal.h"

namespace ImGuiExt {


bool InputText(const char* name, std::string& value, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = 0, void* user_data = 0);


void  GridLine(const char* label, const ImVec2& from, const ImVec2& to, ImU32 col = ImGui::GetColorU32(ImGuiCol_Text));
bool  GridPoint(const char* label, ImVec2& pos, float radius, ImU32 col = ImGui::GetColorU32(ImGuiCol_Text), bool* clicked = 0, bool* selected = 0);

void  BeginTreeNode(const char* name, ImVec2* pos, bool* clicked, bool selected, const ImVec2& size, ImU32 col);
void  EndTreeNode();
int64_t TreeNodeIn(const char* name, uint64_t user_id, uint64_t other_user_id, ImU32 connection_col = ImGui::GetColorU32(ImGuiCol_Text));
int64_t TreeNodeOut(const char* name, uint64_t user_id, uint64_t other_user_id);
void    TreeNodeMarkedConnection(uint64_t a, uint64_t b, float w);

void  TreeNodeConnection(size_t node_from, size_t node_to, size_t out_n, size_t in_n);

void  NodeConnection(const ImVec2& from, const ImVec2& to);

bool  BeginGridView(const char* name, const ImVec2& sz = ImVec2(0, 0));
void  EndGridView();


bool BeginBlendspace(const char* name, ImVec2& sz = ImVec2(0, 0), const ImVec2& snap_step = ImVec2(10, 10));
void EndBlendspace();
bool BlendspacePoint(const char* name, ImVec2& pos, float radius, const ImVec2& snap = ImVec2(0,0), ImU32 col = ImGui::GetColorU32(ImGuiCol_Text));
void BlendspaceLine(const ImVec2& a, const ImVec2& b, ImU32 col = ImGui::GetColorU32(ImGuiCol_Text));


bool BeginTimeline(float length, float* cursor, const ImVec2& sz = ImVec2(0,0));
void EndTimeline();

void TimelineEvent(const char* label, float pos);
void TimelineMarker(const char* label, float pos);


void BeginInsetSegment(ImU32 bg_color = ImGui::GetColorU32(ImGuiCol_FrameBg));
void EndInsetSegment();


}

#endif
