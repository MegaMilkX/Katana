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

        if(ImGuiExt::BeginGridView("Grid")) {
            for(auto n : graph->graph.getNodes()) {
                const gfxm::vec2& p = n->getPos();
                ImVec2 pos(p.x, p.y);
                guiDrawNode(graph->graph, n, &pos);
                n->setPos(gfxm::vec2(pos.x, pos.y));
            }
        }
        ImGuiExt::EndGridView();
    }
    void onGuiToolbox(Editor* ed) override {

    }
};
STATIC_RUN(DocRenderGraph) {
    regEditorDocument<DocRenderGraph>({ "rg" });
}

#endif
