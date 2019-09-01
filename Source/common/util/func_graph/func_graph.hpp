#ifndef FUNC_GRAPH_HPP
#define FUNC_GRAPH_HPP

#include "node_graph.hpp"

#include "../../lib/imgui/imgui.h"

class FuncGraph {
    std::vector<std::shared_ptr<IBaseNode>> nodes;
    std::vector<std::shared_ptr<IFuncNode>> invokable_nodes;
    std::map<IBaseNode*, ImVec2> node_poses;

    void sortInvokeOrder();

public:
    size_t nodeCount() const;
    IBaseNode* getNode(size_t i);

    std::map<IBaseNode*, ImVec2>& getNodePoses();

    template<typename T>
    DataNode<T>* addDataNode(const std::string& name);
    template<typename T>
    ResultNode<T>* addResultNode(const std::string& name);
    std::shared_ptr<IFuncNode> addNode(const std::string& name);

    void connect(IBaseNode* a, IBaseNode* b, size_t out_idx, size_t in_idx);

    void run();
};

template<typename T>
DataNode<T>* FuncGraph::addDataNode(const std::string& name) {
    DataNode<T>* n = new DataNode<T>();
    n->getDesc().name = name;
    nodes.emplace_back(std::shared_ptr<IBaseNode>(n));
    return n;
}
template<typename T>
ResultNode<T>* FuncGraph::addResultNode(const std::string& name) {
    ResultNode<T>* n = new ResultNode<T>();
    n->getDesc().name = name;
    nodes.emplace_back(std::shared_ptr<IBaseNode>(n));
    return n;
}

#endif
