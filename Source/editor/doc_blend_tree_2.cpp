#include "doc_blend_tree_2.hpp"

#include "../common/util/imgui_ext.hpp"

void DocBlendTree2::onGui(Editor* ed, float dt) {
    if(ImGuiExt::BeginGridView("BlendTree2Grid")) {
        ImVec2 pos(0,0);
        ImGuiExt::BeginTreeNode("Test", &pos, 0, false, ImVec2(100, 100), ImGui::ColorConvertFloat4ToU32(ImVec4(.5, 0.2, 0.0, 1.0f)));

        ImGuiExt::TreeNodeIn("test", 0, 0);
        ImGuiExt::TreeNodeOut("result", 0, 0);

        ImGuiExt::EndTreeNode();
    }
    ImGuiExt::EndGridView();
}

void DocBlendTree2::onGuiToolbox(Editor* ed) {
    
}
