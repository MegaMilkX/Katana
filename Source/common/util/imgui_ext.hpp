#ifndef IMGUI_EXT_HPP
#define IMGUI_EXT_HPP

#include "../lib/imgui/imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../lib/imgui/imgui_internal.h"

namespace ImGuiExt {

void BeginTreeNode(const char* name, ImVec2* pos, const ImVec2& size);
void EndTreeNode();
bool TreeNodeIn(const char* name);
bool TreeNodeOut(const char* name);
void TreeNodeConnectionOut(const char* name);
void TreeNodeConnectionIn(const char* out_name);
void CommitTreeNodeConnections();

void NodeConnection(const ImVec2& from, const ImVec2& to);

bool BeginGridView(const char* name);
void EndGridView();

}

#endif
