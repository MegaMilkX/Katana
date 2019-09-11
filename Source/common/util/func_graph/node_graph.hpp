#ifndef FUNC_NODE_GRAPH_HPP
#define FUNC_NODE_GRAPH_HPP

#include <tuple>
#include <type_traits>
#include <stack>

#include "../log.hpp"

/*
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
*/

#include "job_node_desc_lib.hpp"


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
        bind(0, value_ptr);
    }
    template<typename T>
    void bind(size_t i, T* value_ptr) {
        JobOutput& out = outputs[i];
        out.value_ptr = value_ptr;
    }

    virtual void onInvoke() = 0;

public:
    JobNode() {
        desc = JobNodeDescLib::get()->getDesc(rttr::type::get<NODE_T>());
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

#include "../data_stream.hpp"
#include "../data_writer.hpp"
#include "../data_reader.hpp"

class JobGraph {
    std::set<JobGraphNode*> nodes;
    std::map<uint32_t, JobGraphNode*> nodes_by_uid;
    uint32_t next_uid = 0;
    std::vector<JobGraphNode*> invokable_nodes;
public:
    void clear() {
        for(auto j : nodes) {
            delete j;
        }
        nodes.clear();
    }

    std::set<JobGraphNode*>& getNodes() {
        return nodes;
    }

    // Ownership is transferred to the JobGraph
    void addNode(JobGraphNode* job) {
        //assert(job);

        job->setUid(next_uid++);
        nodes.insert(job);
        nodes_by_uid[job->getUid()] = job;
        job->init();
    }

    JobGraphNode* getNode(uint32_t uid) {
        auto it = nodes_by_uid.find(uid);
        if(it == nodes_by_uid.end()) {
            return 0;
        }
        return it->second;
    }

    void prepare() {
        invokable_nodes.clear();
        std::set<JobGraphNode*> valid_nodes;

        
        for(auto job : nodes) {
            if(!job->isInvokable()) {
                continue;
            }
            if(job->isValid()) { // Ignore jobs that lack inputs
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

    void write(out_stream& out) {
        DataWriter w(&out);
        
        std::map<JobGraphNode*, uint32_t> node_id_map;
        std::map<JobOutput*, uint32_t> out_id_map;
        std::vector<JobInput*> inputs;
        for(auto j : nodes) {
            uint32_t id = node_id_map.size();
            node_id_map[j] = id;
            for(size_t i = 0; i < j->inputCount(); ++i) {
                inputs.emplace_back(j->getInput(i));
            }
            for(size_t i = 0; i < j->outputCount(); ++i) {
                uint32_t out_id = out_id_map.size();
                out_id_map[j->getOutput(i)] = out_id;
            }
        }

        w.write(next_uid);
        w.write<uint32_t>(nodes.size());
        for(auto j : nodes) {
            w.write(j->getDesc().name);
            w.write(j->getUid());
            w.write(j->getPos());
        }

        w.write<uint32_t>(inputs.size());
        for(size_t i = 0; i < inputs.size(); ++i) {
            JobInput* in = inputs[i];
            if(in->source) {
                w.write<uint32_t>(out_id_map[in->source]);
            } else {
                w.write<uint32_t>(-1);
            }
        }
    }
    void read(in_stream& in) {
        DataReader r(&in);

        std::vector<JobGraphNode*> nodes_tmp;
        std::vector<JobInput*> ins_tmp;
        std::vector<JobOutput*> outs_tmp;

        next_uid = r.read<uint32_t>();
        uint32_t node_count = r.read<uint32_t>();
        for(uint32_t i = 0; i < node_count; ++i) {
            std::string node_name = r.readStr();
            auto node = createJobNode(node_name);
            node->setUid(r.read<uint32_t>());
            node->setPos(r.read<gfxm::vec2>());
            nodes_tmp.emplace_back(
                node
            );
            for(size_t j = 0; j < node->inputCount(); ++j) {
                ins_tmp.emplace_back(node->getInput(j));
            }
            for(size_t j = 0; j < node->outputCount(); ++j) {
                outs_tmp.emplace_back(node->getOutput(j));
            }
        }

        uint32_t input_count = r.read<uint32_t>();
        for(uint32_t i = 0; i < input_count; ++i) {
            uint32_t out_id = r.read<uint32_t>();
            if(out_id != (uint32_t)-1) {
                ins_tmp[i]->source = outs_tmp[out_id];
            }
        }


        for(auto j : nodes_tmp) {
            nodes.insert(j);
            nodes_by_uid[j->getUid()] = j;
            j->init();
        }

        prepare();
    }
};


#endif
