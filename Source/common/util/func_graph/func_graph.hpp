#ifndef FUNC_GRAPH_HPP
#define FUNC_GRAPH_HPP

#include "node_graph.hpp"

#include "../../lib/imgui/imgui.h"

class FuncGraph {
    std::vector<std::shared_ptr<IBaseNode>> data_nodes;
    std::vector<std::shared_ptr<IFuncNode>> nodes;
    std::map<IFuncNode*, ImVec2> node_poses;

    void sortInvokeOrder();

public:
    size_t dataNodeCount() const;
    IBaseNode* getDataNode(size_t i);
    size_t nodeCount() const;
    IFuncNode* getNode(size_t i);

    std::vector<std::shared_ptr<IFuncNode>>& getNodes();
    std::map<IFuncNode*, ImVec2>& getNodePoses();

    template<typename T>
    void addDataNode(const std::string& name);
    std::shared_ptr<IFuncNode> addNode(const std::string& name);

    void connect(IFuncNode* a, IFuncNode* b, size_t out_idx, size_t in_idx);

    void run();
};

template<typename T>
void FuncGraph::addDataNode(const std::string& name) {

}

#endif
