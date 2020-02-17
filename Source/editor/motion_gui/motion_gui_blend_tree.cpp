#include "motion_gui_blend_tree.hpp"

#include "../../common/util/imgui_ext.hpp"


void MotionGuiBlendTree::guiDrawNode(JobGraph& jobGraph, JobGraphNode* node, ImVec2* pos) {
    bool clicked = false;
    bool selected = selected_node == node;
    std::string node_name = MKSTR(node->getDesc().name << "###" << node);
    gfxm::vec3 col = node->getDesc().color;
    ImGuiExt::BeginTreeNode(node_name.c_str(), pos, &clicked, selected, ImVec2(200, 0), ImGui::ColorConvertFloat4ToU32(ImVec4(col.x, col.y, col.z, 1.0f)));

    int64_t new_conn_tgt = 0;
    for(size_t j = 0; j < node->getDesc().ins.size(); ++j) {
        if(new_conn_tgt = ImGuiExt::TreeNodeIn(node->getDesc().ins[j].name.c_str(), (int64_t)node->getInput(j), (int64_t)node->getInput(j)->source)) {
            node->connect(j, (JobOutput*)new_conn_tgt);
            jobGraph.prepare();
        }
    }
    for(size_t j = 0; j < node->getDesc().outs.size(); ++j) {
        if(new_conn_tgt = ImGuiExt::TreeNodeOut(node->getDesc().outs[j].name.c_str(), (int64_t)node->getOutput(j), 0)) {
            node->connect(j, (JobInput*)new_conn_tgt);
            jobGraph.prepare();
        }
    }

    ImGuiExt::EndTreeNode();
    if(clicked) {
        selected_node = node;
    }
}


MotionGuiBlendTree::MotionGuiBlendTree(const std::string& title, DocMotion* doc, BlendTree* blendTree)
: MotionGuiBase(title, doc), blendTree(blendTree) {

}

void MotionGuiBlendTree::drawGui(Editor* ed, float dt) {
    if(ImGuiExt::BeginGridView("BlendTreeGrid")) {
        for(auto n : blendTree->getNodes()) {
            const gfxm::vec2& p = n->getPos();
            ImVec2 pos(p.x, p.y);
            guiDrawNode(*blendTree, n, &pos);
            n->setPos(gfxm::vec2(pos.x, pos.y));
        }
    }
    ImGuiExt::EndGridView();
}

void MotionGuiBlendTree::drawToolbox(Editor* ed) {
    if(ImGui::Button(ICON_MDI_PLUS " Add node")) {
        ImGui::OpenPopup("test");
    }
    if(ImGui::BeginPopup("test")) {
        if(ImGui::MenuItem("Multiply")) {
            blendTree->addNode(new MultiplyJob);
            blendTree->prepare();
        }
        if(ImGui::MenuItem("Printer")) {
            blendTree->addNode(new PrintJob);
            blendTree->prepare();
        }
        if(ImGui::MenuItem("Test")) {
            blendTree->addNode(new TestJob);
            blendTree->prepare();
        }
        if(ImGui::MenuItem("MotionParam")) {
            blendTree->addNode(new MotionParam);
            blendTree->prepare();
        }
        if(ImGui::MenuItem("Float")) {
            blendTree->addNode(new FloatNode);
            blendTree->prepare();
        }
        if(ImGui::MenuItem("SingleAnim")) {
            blendTree->createNode<SingleAnimJob>();
            blendTree->prepare();
        }
        if(ImGui::MenuItem("Blend3")) {
            blendTree->addNode(new Blend3Job);
            blendTree->prepare();
        }
        ImGui::EndPopup();
    }


    if(selected_node) {
        ImGui::BeginGroup();
        selected_node->onGui();
        ImGui::EndGroup();
    }
}