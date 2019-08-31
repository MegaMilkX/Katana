#include "func_graph.hpp"

#include "../log.hpp"

#include <stack>

void FuncGraph::sortInvokeOrder() {
    std::map<IBaseNode*, int> node_weights;
    for(size_t i = 0; i < nodes.size(); ++i) {
        auto& n = nodes[i];

        std::stack<IBaseNode*> stack;
        IBaseNode* cur = n.get();
        while(cur || !stack.empty()) {
            if(!cur) {
                cur = stack.top();
                stack.pop();
            }

            node_weights[cur]++;

            for(size_t j = 0; j < cur->inputCount(); ++j) {
                size_t out_idx;
                auto in_src = cur->getInputSource(j, out_idx);
                if(in_src) {
                    stack.push(in_src);
                }
            }

            cur = 0;
        }
    }

    std::sort(nodes.begin(), nodes.end(), [&node_weights](const std::shared_ptr<IFuncNode>& a, const std::shared_ptr<IFuncNode>& b)->bool{
        return node_weights[a.get()] > node_weights[b.get()];
    });
}

size_t FuncGraph::dataNodeCount() const {
    return data_nodes.size();
}
IBaseNode* FuncGraph::getDataNode(size_t i) {
    return data_nodes[i].get();
}
size_t FuncGraph::nodeCount() const {
    return nodes.size();
}
IFuncNode* FuncGraph::getNode(size_t i) {
    return nodes[i].get();
}

std::vector<std::shared_ptr<IFuncNode>>& FuncGraph::getNodes() {
    return nodes;
}
std::map<IFuncNode*, ImVec2>& FuncGraph::getNodePoses() {
    return node_poses;
}

std::shared_ptr<IFuncNode> FuncGraph::addNode(const std::string& name) {
    nodes.emplace_back(FuncNodeLib::get()->createNode(name));
    return nodes.back();
}

void FuncGraph::connect(IFuncNode* a, IFuncNode* b, size_t out_idx, size_t in_idx) {
    bool found_a = false;
    bool found_b = false;
    for(size_t i = 0; i < nodes.size(); ++i) {
        if(nodes[i].get() == a) {
            found_a = true;
        } else if(nodes[i].get() == b) {
            found_b = true;
        }
        if(found_a && found_b) {
            break;
        }
    }
    if(!found_a || !found_b) {
        return;
    }

    if(!b->connectInput(in_idx, a, out_idx)) {
        LOG_WARN("Failed to connect func nodes");
    }

    sortInvokeOrder();
}

void FuncGraph::run() {
    for(auto& n : nodes) {
        n->invoke();
    }
}