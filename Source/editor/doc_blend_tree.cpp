#include "doc_blend_tree.hpp"

#include "../common/util/imgui_ext.hpp"

void DocBlendTree::onGui(Editor* ed, float dt) {
    if(ImGuiExt::BeginGridView("BlendTreeGrid")) {
        static ImVec2 node_pos;
        ImGuiExt::BeginTreeNode("Node", &node_pos, ImVec2(100, 0));
        if(ImGuiExt::TreeNodeIn("color")) {
        }
        if(ImGuiExt::TreeNodeIn("alpha")) {

        }
        if(ImGuiExt::TreeNodeOut("out")) {
            
        }
        ImGuiExt::TreeNodeConnectionOut("color_");

        ImGuiExt::EndTreeNode();

        static ImVec2 node_pos2(300, 0);
        ImGuiExt::BeginTreeNode("Node2", &node_pos2, ImVec2(100, 0));
        if(ImGuiExt::TreeNodeIn("color")) {
            
        }
        ImGuiExt::TreeNodeConnectionIn("color_");
        ImGuiExt::TreeNodeConnectionIn("color_2");

        ImGuiExt::EndTreeNode();

        static ImVec2 node_pos3(140, 300);
        ImGuiExt::BeginTreeNode("Node3", &node_pos3, ImVec2(100, 0));
        if(ImGuiExt::TreeNodeOut("noise")) {

        }
        ImGuiExt::TreeNodeConnectionOut("color_2");

        ImGuiExt::EndTreeNode();

        ImGuiExt::CommitTreeNodeConnections();
    }
    ImGuiExt::EndGridView();
}
void DocBlendTree::onGuiToolbox(Editor* ed) {
    
}