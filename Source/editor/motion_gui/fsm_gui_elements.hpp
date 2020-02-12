#ifndef FSM_GUI_ELEMENTS_HPP
#define FSM_GUI_ELEMENTS_HPP

#include "../common/util/imgui_ext.hpp"

#include "../../common/gfxm.hpp"

#include "../../common/util/materialdesign_icons.hpp"
#include "../../common/util/log.hpp"

#include <string>


const int NODE_FLAG_CLIP = 1;
const int NODE_FLAG_FSM = 2;
const int NODE_FLAG_BLEND_TREE = 4;
const int NODE_FLAG_HIGHLIGHT = 8;
const int NODE_FLAG_SELECTED = 16;

ImVec2 GraphEditGridPosToScreen(const ImVec2& pos);

ImVec2 GraphEditGridScreenToPos(const ImVec2& pos);

bool TransitionLine(const ImVec2& from, const ImVec2& to, bool selected = false);

bool Node(const char* id, ImVec2& pos, const ImVec2& node_size, int node_flags, bool* double_clicked = 0);

bool BeginGridView(const char* id);
void EndGridView();


#endif
