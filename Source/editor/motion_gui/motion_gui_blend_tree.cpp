#include "motion_gui_blend_tree.hpp"

#include "../../common/util/imgui_ext.hpp"


void MotionGuiBlendTree::guiDrawNode(JobGraph& jobGraph, JobGraphNode* node, ImVec2* pos) {
    bool clicked = false;
    bool selected = selected_node == node;
    std::string node_name = MKSTR(node->getName() << " (" << node->getDesc().name << ")###" << node);
    gfxm::vec3 col = node->getDesc().color;
    ImGuiExt::BeginTreeNode(node_name.c_str(), pos, &clicked, selected, ImVec2(200, 0), ImGui::ColorConvertFloat4ToU32(ImVec4(col.x, col.y, col.z, 1.0f)));

    int64_t new_conn_tgt = 0;
    for(size_t j = 0; j < node->getDesc().ins.size(); ++j) {
        ImU32 col = ImGui::GetColorU32(ImGuiCol_Text, .5f);
        if(node->getInput(j)->influence > .0f) {
            col = ImGui::GetColorU32(ImGuiCol_Text);
        }
        if(new_conn_tgt = ImGuiExt::TreeNodeIn(node->getDesc().ins[j].name.c_str(), (int64_t)node->getInput(j), (int64_t)node->getInput(j)->source, col)) {
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
/*
    for(int i = 0; i < jobGraph.markedConnectionCount(); ++i) {
        auto& c = jobGraph.getMarkedConnection(i);
        ImGuiExt::TreeNodeMarkedConnection((uint64_t)c.out, (uint64_t)c.in, c.weight);
    }*/

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
    ImGui::PushItemWidth(-1);
    if(ImGui::BeginCombo("###AddJobNode", ICON_MDI_PLUS " Add node...")) {
        for(int i = 0; i < JobNodeDescLib::get()->descCount(); ++i) {
            FuncNodeDesc* desc = JobNodeDescLib::get()->getDesc(i);
            if(desc->graph_type != rttr::type::get<JobGraph>() && desc->graph_type != rttr::type::get<BlendTree>()) {
                continue;
            }
            if(ImGui::Selectable(desc->name.c_str())) {
                blendTree->addNode(desc->node_constructor());
                blendTree->prepare();
            }
        }

        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    if(selected_node) {
        ImGui::BeginGroup();
        char buf[256];
        memset(buf, 0, sizeof(buf));
        memcpy(buf, selected_node->getName().data(), selected_node->getName().size());
        if(ImGui::InputText("name", buf, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
            selected_node->setName(buf);
        }
        if(ImGui::Button(ICON_MDI_CLOSE " Remove node", ImVec2(ImGui::GetWindowContentRegionWidth(), 0))) {
            blendTree->removeNode(selected_node);
            selected_node = 0;
        }
        if(selected_node) {
            selected_node->onGui();
        }
        ImGui::EndGroup();
    }
}