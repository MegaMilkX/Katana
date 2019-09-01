#ifndef I_FUNC_NODE_HPP
#define I_FUNC_NODE_HPP

#include "func_node_desc.hpp"


class IBaseNode {
public:
    virtual FuncNodeDesc& getDesc() = 0;

    virtual size_t inputCount() const = 0;
    virtual IBaseNode* getInputSource(size_t input_idx, size_t& src_output_idx) = 0;
    virtual size_t outputCount() const = 0;
    virtual void* getOutputPtr(size_t idx) = 0;

    virtual bool connectInput(size_t in_index, IBaseNode* node, size_t out_index) = 0;

};

class IFuncNode : public IBaseNode {
public:
    virtual ~IFuncNode() {}
    virtual IFuncNode* clone() const = 0;

    virtual void invoke(void) = 0;
};

#endif
