#ifndef JOB_GRAPH_NODE_HPP
#define JOB_GRAPH_NODE_HPP

#include <rttr/type>
#include <rttr/registration>
#include <vector>

#include "../../gfxm.hpp"

class JobGraphNode;
struct JobOutput;
struct JobInput {
    JobOutput* source = 0;
    rttr::type type;
    JobGraphNode* owner = 0;
};
struct JobOutput {
    void* value_ptr = 0;
    rttr::type type;
    JobGraphNode* owner = 0;
};

class FuncNodeDesc;

class JobGraphNode {
    RTTR_ENABLE()
protected:
    uint32_t uid = 0;
    FuncNodeDesc* desc = 0;
    std::vector<JobInput> inputs;
    std::vector<JobOutput> outputs;

    gfxm::vec2 visual_pos;

    virtual void onInit() = 0;

public:
    void setUid(uint32_t uid) { this->uid = uid; }
    uint32_t getUid() const { return uid; }
    FuncNodeDesc& getDesc() { return *desc; }
    size_t inputCount() const { return inputs.size(); }
    size_t outputCount() const { return outputs.size(); }
    JobInput* getInput(size_t i) { return &inputs[i]; }
    JobOutput* getOutput(size_t i) { return &outputs[i]; }
    const gfxm::vec2& getPos() const { return visual_pos; }
    void setPos(const gfxm::vec2& pos) { visual_pos = pos; } 

    bool connect(size_t input, JobOutput* source) {
        if(input >= inputs.size()) {
            return false;
        }
        if(inputs[input].type != source->type) {
            return false;
        }
        inputs[input].source = source;
        return true;
    }
    bool connect(size_t output, JobInput* target) {
        if(output >= outputs.size()) {
            return false;
        }
        if(outputs[output].type != target->type) {
            return false;
        }
        target->source = &outputs[output];
        return true;
    }

    virtual bool isInvokable() { return false; }
    bool isValid() {
        for(size_t i = 0; i < inputCount(); ++i) {
            if(!getInput(i)->source) {
                return false;
            }
        }
        return true;
    }

    void init() {
        onInit();
    }
    virtual void invoke() {}

    virtual void onGui() {}
};

#endif
