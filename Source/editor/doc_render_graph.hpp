#ifndef DOC_RENDER_GRAPH_HPP
#define DOC_RENDER_GRAPH_HPP

#include "editor_document.hpp"
#include "../common/resource/render_graph.hpp"
#include "../common/util/imgui_ext.hpp"


class DocRenderGraph : public EditorDocumentTyped<RenderGraph> {
    JobGraphNode* selected_node = 0;

    void guiDrawNode(JobGraph& jobGraph, JobGraphNode* node, ImVec2* pos) {
        bool clicked = false;
        bool selected = selected_node == node;
        std::string node_name = MKSTR(node->getDesc().name << "###" << node);
        gfxm::vec3 col = node->getDesc().color;
        ImGuiExt::BeginTreeNode(node_name.c_str(), pos, &clicked, selected, ImVec2(200, 0), ImGui::ColorConvertFloat4ToU32(ImVec4(col.x, col.y, col.z, 1.0f)));

        void* new_conn_tgt = 0;
        for(size_t j = 0; j < node->getDesc().ins.size(); ++j) {
            if(new_conn_tgt = ImGuiExt::TreeNodeIn(node->getDesc().ins[j].name.c_str(), (void*)node->getInput(j), (void*)node->getInput(j)->source)) {
                node->connect(j, (JobOutput*)new_conn_tgt);
                jobGraph.prepare();
            }
        }
        for(size_t j = 0; j < node->getDesc().outs.size(); ++j) {
            if(new_conn_tgt = ImGuiExt::TreeNodeOut(node->getDesc().outs[j].name.c_str(), (void*)node->getOutput(j), 0)) {
                node->connect(j, (JobInput*)new_conn_tgt);
                jobGraph.prepare();
            }
        }

        ImGuiExt::EndTreeNode();
        if(clicked) {
            selected_node = node;
        }
    }

public:
    void onGui(Editor* ed, float dt) override {
        auto& graph = _resource;
        graph->prepare();
        graph->run();

        auto graph_root = graph->getRoot();

        ImGui::BeginColumns("First", 2);
        
        if(ImGuiExt::BeginGridView("Grid")) {
            for(auto n : graph->getNodes()) {
                const gfxm::vec2& p = n->getPos();
                ImVec2 pos(p.x, p.y);
                guiDrawNode(*graph, n, &pos);
                n->setPos(gfxm::vec2(pos.x, pos.y));
            }
        }
        ImGuiExt::EndGridView();

        ImGui::NextColumn();

        ImGui::BeginChild("RenderResult");
        if(graph_root->isValid()) {
            auto fb = graph_root->getFrameBuffer();
            GLuint texid = fb->getTextureId(0);

            ImVec2 winMin = ImGui::GetWindowContentRegionMin();
            ImVec2 winMax = ImGui::GetWindowContentRegionMax();
            ImVec2 winSize = ImVec2(winMax - winMin);
            ImGui::Image(
                (ImTextureID)texid, 
                winSize,
                ImVec2(0, 1), ImVec2(1, 0)
            );
        }
        ImGui::EndChild();

        ImGui::EndColumns();
    }
    void onGuiToolbox(Editor* ed) override {
        auto& blendTree = _resource;

        if(ImGui::Button(ICON_MDI_PLUS " Add node")) {
            ImGui::OpenPopup("test");
        }
        if(ImGui::BeginPopup("test")) {
            if(ImGui::MenuItem("Texture2d")) {
                blendTree->addNode(new RenderJobTexture2d);
            }
            if(ImGui::MenuItem("FrameBuffer")) {
                blendTree->addNode(new RenderJobFrameBuffer);
            }
            if(ImGui::MenuItem("Clear")) {
                blendTree->addNode(new RenderJobClear);
            }
            ImGui::EndPopup();
        }


        if(selected_node) {
            ImGui::BeginGroup();
            selected_node->onGui();
            ImGui::EndGroup();
        }
    }
};
STATIC_RUN(DocRenderGraph) {
    regEditorDocument<DocRenderGraph>({ "rg" });
}

#endif
