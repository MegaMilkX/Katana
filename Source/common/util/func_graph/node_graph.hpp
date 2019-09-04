#ifndef FUNC_NODE_GRAPH_HPP
#define FUNC_NODE_GRAPH_HPP

#include <tuple>
#include <type_traits>

#include "i_func_node.hpp"


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


#include "func_node_lib.hpp"

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


struct JobOutputDesc {
    std::string name;
    rttr::type type;
};
struct JobInputDesc {
    std::string name;
    rttr::type type;
};


class JobDescMaker {
    std::string _name;
    std::vector<JobInputDesc> inputs;
    std::vector<JobOutputDesc> outputs;  
public:
    JobDescMaker& name(const std::string& name) {
        this->_name = name;
        return *this;
    }
    template<typename T>
    JobDescMaker& in(const std::string& name, unsigned int count = 1) {
        for(unsigned int i = 0; i < count; ++i) {
            JobInputDesc in{ name, rttr::type::get<T>() };
            inputs.emplace_back(in);
        }
        return *this;
    }
    template<typename T>
    JobDescMaker& out(const std::string& name, int count = 1) {
        for(unsigned int i = 0; i < count; ++i) {
            JobOutputDesc out{ name, rttr::type::get<T>() };
            outputs.emplace_back(out);
        }
        return *this;
    }

    FuncNodeDesc compile() {
        FuncNodeDesc desc;
        desc.name = _name;
        for(auto& i : inputs) {
            desc.ins.emplace_back(FuncNodeDesc::In{ i.name, i.type });
        }
        for(auto& o : outputs) {
            desc.outs.emplace_back(FuncNodeDesc::Out{ o.name, o.type });
        }
        return desc;
    }
};


struct JobOutput;
struct JobInput {
    JobOutput* source = 0;
};
struct JobOutput {
    std::vector<char> buf;
};


class JobNode {
protected:
    FuncNodeDesc desc;
    std::vector<char> out_buf;
    std::vector<JobInput> inputs;
    std::vector<JobOutput> outputs;

    template<typename T>
    T get(size_t i) {
        return T();
    }
    template<typename T>
    void emit(const T& value) {

    }
    template<typename T>
    void emit(size_t i, const T& value) {

    }

public:
    void init() {
        JobDescMaker maker;
        onMakeDesc(maker);
        desc = maker.compile();
    }

    FuncNodeDesc& getDesc() { return desc; }
    size_t inputCount() const { return inputs.size(); }
    size_t outputCount() const { return outputs.size(); }
    JobInput* getInput(size_t i) { return &inputs[i]; }
    JobOutput* getOutput(size_t i) { return &outputs[i]; }

    bool connect(size_t input, JobOutput* source);

    void invoke() {
        onInvoke();
    }

    virtual void onMakeDesc(JobDescMaker& descMaker) = 0;
    virtual void onInvoke() = 0;

    
};

class MultiplyJob : public JobNode {
public:
    void onMakeDesc(JobDescMaker& descMaker) override {
        descMaker
            .name("Multiply")
            .in<float>("float", 2)
            .out<int>("result");
    }

    void onInvoke() override {
        auto a = get<float>(0);
        auto b = get<float>(1);

        // TODO: Do stuff

        emit<float>(0, 10.0f);
    }
};


#endif
