#ifndef FUNC_NODE_LIB_HPP
#define FUNC_NODE_LIB_HPP

#include <memory>
#include <map>
#include <string>

#include "../singleton.hpp"

#include "i_func_node.hpp"

class FuncNodeLib : public Singleton<FuncNodeLib> {
    std::map<
        std::string,
        std::shared_ptr<IFuncNode>
    > nodes;
public:
    std::shared_ptr<IFuncNode> createNode(const std::string& name) {
        auto it = nodes.find(name);
        if(it == nodes.end()) {
            return 0;
        }
        return std::shared_ptr<IFuncNode>(it->second->clone());
    }

    std::shared_ptr<IFuncNode> getMemberNode(const std::string& name) {

    }
    
    void regNode(const std::string& name, const std::shared_ptr<IFuncNode>& node) {
        nodes[name] = node;
    } 
    
};

#endif
