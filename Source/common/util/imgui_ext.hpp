#ifndef IMGUI_EXT_HPP
#define IMGUI_EXT_HPP

#include "../lib/imgui/imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../lib/imgui/imgui_internal.h"

namespace ImGuiExt {

void  BeginTreeNode(const char* name, ImVec2* pos, bool* clicked, bool selected, const ImVec2& size);
void  EndTreeNode();
void* TreeNodeIn(const char* name, void* user_id, void* other_user_id);
void* TreeNodeOut(const char* name, void* user_id, void* other_user_id);

void  TreeNodeConnection(size_t node_from, size_t node_to, size_t out_n, size_t in_n);

void  NodeConnection(const ImVec2& from, const ImVec2& to);

bool  BeginGridView(const char* name);
void  EndGridView();

}

#endif
