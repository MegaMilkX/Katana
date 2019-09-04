#ifndef I_FUNC_NODE_HPP
#define I_FUNC_NODE_HPP

#include "func_node_desc.hpp"

class IBaseNode;
struct NodeInput {
    IBaseNode* src_node = 0;
    size_t src_out_index;
};

class IBaseNode {
protected:
    FuncNodeDesc desc;
    std::vector<NodeInput> in_connections;

public:
    FuncNodeDesc& getDesc() {
        return desc;
    }

    size_t inputCount() const {
        return in_connections.size();
    }
    NodeInput* getInput(size_t i) {
        return &in_connections[i];
    }
    IBaseNode* getInputSource(size_t input_index, size_t& source_output_index) {
        source_output_index = in_connections[input_index].src_out_index;
        return in_connections[input_index].src_node;
    }
    size_t outputCount() const {
        return desc.outs.size();
    }
    virtual void* getOutputPtr(size_t idx) = 0;

    bool connectInput(size_t in_index, IBaseNode* node, size_t out_index) {
        if(in_index >= desc.ins.size()) {
            return false;
        }

        auto& ins = desc.ins;        
        auto& other_outs = node->getDesc().outs;
        if(out_index >= other_outs.size()) {
            return false;
        }
        auto& out = other_outs[out_index];
        if(out.type != ins[in_index].type) {
            return false;
        }

        in_connections[in_index].src_node = node;
        in_connections[in_index].src_out_index = out_index;

        return true;
    }

};

class IFuncNode : public IBaseNode {
public:
    virtual ~IFuncNode() {}
    virtual IFuncNode* clone() const = 0;

    virtual void invoke(void) = 0;
};

#endif
