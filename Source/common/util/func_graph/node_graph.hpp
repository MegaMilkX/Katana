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
    FuncNodeDesc desc;
    struct InConnection {
        IBaseNode* src_node = 0;
        size_t src_out_index;
    };
    InConnection in;

public:
    ResultNode() {
        FuncNodeDesc::In in = {
            "result",
            rttr::type::get<T>()
        };
        desc.ins.emplace_back(in);
    }

    T get() {
        T* ptr = 0;
        auto& conn = in;
        if(conn.src_node) {
            ptr = (T*)conn.src_node->getOutputPtr(conn.src_out_index);
        }
        if(!ptr) {
            return T();
        }
        return *ptr;
    }

    FuncNodeDesc& getDesc() override {
        return desc;
    }
    size_t inputCount() const override {
        return 1;
    }
    IBaseNode* getInputSource(size_t input_idx, size_t& out_idx) override {
        if(input_idx != 0) {
            return 0;
        }
        out_idx = in.src_out_index;
        return in.src_node;
    }
    size_t outputCount() const override {
        return 0;
    }
    void* getOutputPtr(size_t i) override {
        return 0;
    }
    bool connectInput(size_t in_index, IBaseNode* node, size_t out_idx) override {
        if(in_index >= desc.ins.size()) {
            return false;
        }

        auto& ins = desc.ins;        
        auto& other_outs = node->getDesc().outs;
        if(out_idx >= other_outs.size()) {
            return false;
        }
        auto& out = other_outs[out_idx];
        if(out.type != ins[in_index].type) {
            return false;
        }

        in.src_node = node;
        in.src_out_index = out_idx;

        return true;
    }
};

template<typename T>
class DataNode : public IBaseNode {
    T value;
    FuncNodeDesc desc;
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

    FuncNodeDesc& getDesc() override {
        return desc;
    }
    size_t inputCount() const override {
        return 0;
    }
    IBaseNode* getInputSource(size_t input_idx, size_t& out_idx) override {
        return 0;
    }
    size_t outputCount() const override {
        return 1;
    }
    void* getOutputPtr(size_t i) override {
        return (void*)&value;
    }

    bool connectInput(size_t in_index, IBaseNode* node, size_t out_idx) override {
        return false;
    }


};

template<typename RET, typename... Args>
class FuncNode : public IFuncNode {
    template<typename T>
    struct ArgPack {
        T value;
        T& get(FuncNode* node, int index) {
            auto& desc = node->getDesc();
            auto& info = desc.arg_infos[index];
            T* ptr = 0;
            if(info.arg_type == ARG_OUT) {
                ptr = (T*)(node->out_buf.data() + desc.outs[info.in_out_index].buf_offset);
                //ptr = &value;
            } else if(info.arg_type == ARG_IN) {
                auto& conn = node->in_connections[info.in_out_index];
                if(!conn.src_node) {
                    ptr = &value;
                } else {
                    ptr = (T*)conn.src_node->getOutputPtr(conn.src_out_index);
                    if(!ptr) {
                        ptr = &value;
                    }
                }
            }
            return *ptr;
        }
    };
    friend ArgPack;

    struct InConnection {
        IBaseNode* src_node = 0;
        size_t src_out_index;
    };
    
    RET(*func)(Args...);
    FuncNodeDesc desc;
    std::vector<InConnection> in_connections;
    
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

    FuncNodeDesc& getDesc() override {
        return desc;
    } 

    size_t outputCount() const override {
        return desc.outs.size();
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

    size_t inputCount() const override {
        return in_connections.size();
    }
    IBaseNode* getInputSource(size_t input_index, size_t& source_output_index) override {
        source_output_index = in_connections[input_index].src_out_index;
        return in_connections[input_index].src_node;
    }

    bool connectInput(size_t in_index, IBaseNode* node, size_t out_index) override {
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


template<typename CLASS, typename RET, typename... Args>
class MemberFuncNode {
    typedef RET(CLASS::*func_t)(Args...);
public:
    MemberFuncNode(func_t func);
};

class Blend3 : public MemberFuncNode<Blend3, void, float> {
public:
};


template<typename CLASS, typename RET, typename... Args>
void regFuncNode(const std::string& path, RET(CLASS::*func)(Args...)) {

}


#endif
