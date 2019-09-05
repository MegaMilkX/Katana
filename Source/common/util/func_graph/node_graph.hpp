#ifndef FUNC_NODE_GRAPH_HPP
#define FUNC_NODE_GRAPH_HPP

#include <tuple>
#include <type_traits>
#include <stack>

#include "i_func_node.hpp"

#include "../log.hpp"


template<typename T>
struct bar {
    static ArgInfo get() {
        return { rttr::type::get<T>(), sizeof(T), ARG_IN };
    }
};
template<typename T>
struct bar<T&> {
    static ArgInfo get() {
        return { rttr::type::get<T>(), sizeof(T), ARG_OUT };
    }
};
template<typename T>
struct bar<const T&> {
    static ArgInfo get() {
        return { rttr::type::get<T>(), sizeof(T), ARG_IN };
    }
};
template<typename T>
struct bar<T*> {
    static ArgInfo get() {
        return { rttr::type::get<T*>(), sizeof(T*), ARG_IN };
    }
};
template<typename T>
struct bar<const T*> {
    static ArgInfo get() {
        return { rttr::type::get<T*>(), sizeof(T*), ARG_IN };
    }
};


#include "genseq.hpp"

template<typename... Args, int... S>
void iterate_args(FuncNodeDesc& desc, seq<S...>) {
    //std::tuple<typename std::remove_reference<typename std::remove_const<Args>::type>::type...> pack;
    ArgInfo array[] = { bar<Args>::get()... };

    size_t buf_offset = 0;
    size_t in_index = 0;
    size_t out_index = 0;
    for(size_t i = 0; i < sizeof(array) / sizeof(array[0]); ++i) {
        ArgInfo& info = array[i];
        if(info.arg_type == ARG_IN) {
            desc.ins.push_back({
                info.type.get_name().to_string(),
                info.type
            });
            info.in_out_index = in_index;
            ++in_index;
        } else if(info.arg_type == ARG_OUT) {
            desc.outs.push_back({
                info.type.get_name().to_string(),
                info.type,
                buf_offset,
                info.sz
            });
            info.in_out_index = out_index;
            ++out_index;
            buf_offset += info.sz;
        }
    }
    desc.out_buf_sz = buf_offset;
    desc.arg_infos = std::vector<ArgInfo>(array, array + sizeof(array) / sizeof(array[0]));
}
template<typename RET, typename... Args>
FuncNodeDesc makeFuncNodeDesc(RET(*func)(Args...)) {
    FuncNodeDesc desc;
    iterate_args<Args...>(desc, typename genseq<sizeof...(Args)>::type());
    return desc;
}

template<typename T>
class ResultNode : public IBaseNode {
public:
    ResultNode() {
        FuncNodeDesc::In in = {
            "result",
            rttr::type::get<T>()
        };
        desc.ins.emplace_back(in);
        in_connections.resize(1);
    }

    T get() {
        T* ptr = 0;
        auto conn = getInput(0);
        if(conn->src_node) {
            ptr = (T*)conn->src_node->getOutputPtr(conn->src_out_index);
        }
        if(!ptr) {
            return T();
        }
        return *ptr;
    }

    void* getOutputPtr(size_t i) override {
        return 0;
    }
};

template<typename T>
class DataNode : public IBaseNode {
    T value;
public:
    DataNode() {
        FuncNodeDesc::Out out = {
            "out",
            rttr::type::get<T>(),
            0,
            sizeof(T)
        };
        desc.outs.emplace_back(out);
        
    }

    void set(const T& value) {
        this->value = value;
    }
    T& get() {
        return value;
    }

    void* getOutputPtr(size_t i) override {
        return (void*)&value;
    }
};


template<typename T>
struct ArgPack {
    T value;
    T& get(IFuncNode* node, int index) {
        auto& desc = node->getDesc();
        auto& info = desc.arg_infos[index];
        T* ptr = 0;
        if(info.arg_type == ARG_OUT) {
            ptr = (T*)node->getOutputPtr(info.in_out_index);
            //ptr = &value;
        } else if(info.arg_type == ARG_IN) {
            auto conn = node->getInput(info.in_out_index);
            if(!conn->src_node) {
                ptr = &value;
            } else {
                ptr = (T*)conn->src_node->getOutputPtr(conn->src_out_index);
                if(!ptr) {
                    ptr = &value;
                }
            }
        }
        return *ptr;
    }
};


template<typename RET, typename... Args>
class FuncNode : public IFuncNode {    
    RET(*func)(Args...);
    
    std::vector<char> out_buf;
    std::tuple<ArgPack<typename std::remove_reference<typename std::remove_const<Args>::type>::type>...> pack;

    template<int... S>
    void invoke_impl(seq<S...>) {
        if(func) {
            func(std::get<S>(pack).get(this, S)...);
        }
    }
public:
    FuncNode(const FuncNodeDesc& desc, RET(*func)(Args...)) 
    : func(func) {
        this->desc = desc;
        in_connections.resize(desc.ins.size());
        out_buf.resize(desc.out_buf_sz);
    }
    IFuncNode* clone() const override {
        return new FuncNode(desc, func);
    }
    
    void* getOutputPtr(size_t i) override {
        if(i >= desc.outs.size()) {
            return 0;
        }
        return ((char*)out_buf.data()) + desc.outs[i].buf_offset;
    }
    void* getOutBuf() {
        return (void*)out_buf.data();
    }

    void invoke() override {
        invoke_impl(typename genseq<sizeof...(Args)>::type());
    }
};

template<typename RET, typename... Args>
std::shared_ptr<IFuncNode> createFuncNode(const FuncNodeDesc& desc, RET(*func)(Args...)) {
    return std::shared_ptr<IFuncNode>(new FuncNode<RET, Args...>(desc, func));
}


template<typename RET, typename... Args>
void regFuncNode(const std::string& name, RET(*func)(Args...), const std::vector<std::string>& arg_names = {}) {
    FuncNodeDesc desc = makeFuncNodeDesc(func);
    desc.name = name;
    size_t in_idx = 0;
    size_t out_idx = 0;
    for(size_t i = 0; i < arg_names.size() && i < desc.arg_infos.size(); ++i) {
        ARG_TYPE arg_type = desc.arg_infos[i].arg_type;
        if(arg_type == ARG_IN) {
            desc.ins[in_idx].name = arg_names[i];
            ++in_idx;
        } else if(arg_type == ARG_OUT) {
            desc.outs[out_idx].name = arg_names[i];
            ++out_idx;
        }
    }
    
    FuncNodeLib::get()->regNode(name, createFuncNode(desc, func));
    
}


template<typename T>
FuncNodeDesc::In getInputDesc() {
    FuncNodeDesc::In in;
    in.type = rttr::type::get<T>();
}


template<typename RET, typename... Args>
class FuncNode_ : public IFuncNode {
    typedef RET(*func_t)(Args...);
    

    func_t func;
    RET ret_value;
    std::tuple<ArgPack<typename std::remove_reference<typename std::remove_const<Args>::type>::type>...> pack;

    template<int... S>
    void iterate_args(seq<S...>) {
        int arg_count = sizeof...(Args);
        desc.ins = { getInputDesc<Args>()... };

        desc.out_buf_sz = sizeof(RET);
        desc.outs.emplace_back(
            FuncNodeDesc::Out{
                "",
                rttr::type::get<RET>(),
                0,
                sizeof(RET)
            }
        );
        in_connections.resize(arg_count);
    }

    template<int... S>
    void invoke_impl(seq<S...>) {
        ret_value = func(std::get<S>(pack).get(this, S)...);
    }

public:
    FuncNode_(func_t f)
    : func(f) {
        iterate_args<Args...>(typename genseq<sizeof...(Args)>::type());
    }

    void invoke() override {
        invoke_impl(typename genseq<sizeof...(Args)>::type());
    }
};


template<typename CLASS, typename RET, typename... Args>
class MemberFuncNode {
    typedef RET(CLASS::*func_t)(Args...);
public:
    MemberFuncNode(func_t func);
};


#include "job_node_desc_lib.hpp"


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


class JobGraphNode {
protected:
    FuncNodeDesc* desc = 0;
    std::vector<JobInput> inputs;
    std::vector<JobOutput> outputs;

    virtual void onInit() = 0;

public:
    FuncNodeDesc& getDesc() { return *desc; }
    size_t inputCount() const { return inputs.size(); }
    size_t outputCount() const { return outputs.size(); }
    JobInput* getInput(size_t i) { return &inputs[i]; }
    JobOutput* getOutput(size_t i) { return &outputs[i]; }

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

    void init() {
        onInit();
    }
    virtual void invoke() {}
};


template<typename NODE_T>
class JobNode : public JobGraphNode {
protected:
    template<typename T>
    T& get(size_t i) {
        JobInput& in = inputs[i];
        return *(T*)in.source->value_ptr;
    }
    template<typename T>
    void bind(T* value_ptr) {
        emit(0, value);
    }
    template<typename T>
    void bind(size_t i, T* value_ptr) {
        JobOutput& out = outputs[i];
        out.value_ptr = value_ptr;
    }

    virtual void onInvoke() = 0;

public:
    JobNode() {
        desc = JobNodeDescLib::get()->getDesc(rttr::type::get<NODE_T>().get_name().to_string());
        for(auto& in : desc->ins) {
            inputs.emplace_back(JobInput{ 0, in.type, this });
        }
        for(auto& out : desc->outs) {
            outputs.emplace_back(JobOutput{ 0, out.type, this });
        }
    }

    bool isInvokable() override { return true; }

    void invoke() override {
        onInvoke();
    }    
};

class MultiplyJob : public JobNode<MultiplyJob> {
    float result = .0f;
public:
    void onInit() override {
        bind<float>(0, &result);
    }

    void onInvoke() override {
        auto a = get<float>(0);
        auto b = get<float>(1);

        result = a * b;
    }
};

class TestJob : public JobNode<TestJob> {
    float val = 100.0f;
public:
    void onInit() override {
        bind<float>(0, &val);
    }

    void onInvoke() override {
        val += 1.0f/60.0f;
    }
};

class PrintJob : public JobNode<PrintJob> {
public:
    void onInit() override {}

    void onInvoke() override {
        auto a = get<float>(0);
        LOG(a);
    }
};


class JobGraph {
    std::set<JobGraphNode*> nodes;
    std::vector<JobGraphNode*> invokable_nodes;
public:
    std::set<JobGraphNode*>& getNodes() {
        return nodes;
    }

    // Ownership is transferred to the JobGraph
    void addNode(JobGraphNode* job) {
        nodes.insert(job);
        job->init();
    }

    void prepare() {
        invokable_nodes.clear();
        std::set<JobGraphNode*> valid_nodes;

        // Ignore jobs that lack inputs
        for(auto job : nodes) {
            if(!job->isInvokable()) {
                continue;
            }
            bool valid_job = true;
            for(size_t i = 0; i < job->inputCount(); ++i) {
                if(!job->getInput(i)->source) {
                    valid_job = false;
                    break;
                }
            }
            if(valid_job) {
                valid_nodes.insert(job);
                invokable_nodes.emplace_back(job);
            }
        }

        // Sort execution order
        std::map<JobGraphNode*, int> node_weights;
        for(auto n : valid_nodes) {
            std::stack<JobGraphNode*> stack;
            JobGraphNode* cur = n;
            while(cur || !stack.empty()) {
                if(!cur) {
                    cur = stack.top();
                    stack.pop();
                }

                node_weights[cur]++;

                for(size_t j = 0; j < cur->inputCount(); ++j) {
                    auto in_src = cur->getInput(j)->source->owner;
                    if(valid_nodes.count(in_src) == 0) {
                        continue;
                    }
                    if(!in_src) {
                        continue;
                    }

                    stack.push(in_src);
                }

                cur = 0;
            }
        }

        std::sort(invokable_nodes.begin(), invokable_nodes.end(), [&node_weights](JobGraphNode* a, JobGraphNode* b)->bool{
            return node_weights[a] > node_weights[b];
        });
    }

    void run() {
        for(auto job : invokable_nodes) {
            job->invoke();
        }
    }
};


#endif
