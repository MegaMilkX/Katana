#ifndef FUNC_NODE_GRAPH_HPP
#define FUNC_NODE_GRAPH_HPP

#include <tuple>
#include <type_traits>
#include <memory>

#include <rttr/type>

enum ARG_TYPE {
    ARG_NONE,
    ARG_IN,
    ARG_OUT
};
struct ArgInfo {
    rttr::type type;
    size_t sz;
    ARG_TYPE arg_type;
    size_t in_out_index;
};

struct FuncNodeDesc {
    struct In {
        std::string name;
        rttr::type type;
    };
    struct Out {
        std::string name;
        rttr::type type;
        size_t buf_offset;
        size_t size;
    };
    std::string name;
    std::vector<In> ins;
    std::vector<Out> outs;
    std::vector<ArgInfo> arg_infos;
    size_t out_buf_sz;
};

class IFuncNode {
public:
    virtual ~IFuncNode() {}
    virtual const FuncNodeDesc& getDesc() const = 0;

    virtual void* getOutBuf() = 0;

    virtual size_t inputCount() const = 0;
    virtual IFuncNode* getInputSource(size_t input_index, size_t& source_output_index) = 0;
    virtual bool connectInput(size_t in_index, IFuncNode* node, size_t out_index) = 0;

    virtual void invoke(void) = 0;
};

template<typename T>
int foo(const T& v) {
    LOG(rttr::type::get<T>().get_name().to_string());
    return 0;
}


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


template<int...>
struct seq {};
template<int N, int... S>
struct genseq : genseq<N-1, N-1, S...> {};
template<int... S>
struct genseq<0, S...> {
    typedef seq<S...> type;
};

template<typename... Args, int... S>
void iterate_args(FuncNodeDesc& desc, seq<S...>) {
    std::tuple<typename std::remove_reference<typename std::remove_const<Args>::type>::type...> pack;
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
template<typename RET, typename... Args>
FuncNodeDesc makeFuncNodeDesc(const std::string& name, RET(*func)(Args...), const std::vector<std::string>& arg_names = {}) {
    FuncNodeDesc desc = makeFuncNodeDesc(func);
    desc.name = name;
    
    return desc;
}

#include "singleton.hpp"

class FuncNodeLib : public Singleton<FuncNodeLib> {
public:
    std::map<std::string, FuncNodeDesc> descs;
};

template<typename RET, typename... Args>
void regFuncNode(const std::string& name, RET(*func)(Args...), const std::vector<std::string>& arg_names = {}) {
    FuncNodeDesc desc = makeFuncNodeDesc(func);
    desc.name = name;
    FuncNodeLib::get()->descs[name] = desc;
}

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
            } else if(info.arg_type == ARG_IN) {
                ptr = (T*)node->in_connections_ptrs[info.in_out_index];
                if(!ptr) {
                    ptr = &value;
                }
            }
            return *ptr;
        }
    };
    friend ArgPack;

    struct InConnection {
        IFuncNode* src_node = 0;
        size_t src_out_index;
    };
    
    RET(*func)(Args...);
    FuncNodeDesc desc;
    std::vector<void*> in_connections_ptrs;
    std::vector<InConnection> in_connections;
    
    std::vector<char> out_buf;
    std::tuple<ArgPack<typename std::remove_reference<typename std::remove_const<Args>::type>::type>...> pack;
    
    template<int... S>
    void iterate_args(seq<S...>) {
        ArgInfo array[] = { bar<Args>::get()... };
        for(size_t i = 0; i < sizeof(array) / sizeof(array[0]); ++i) {
            
        }
    }

    template<int... S>
    void invoke_impl(seq<S...>) {
        if(func) {
            func(std::get<S>(pack).get(this, S)...);
        }
    }
public:
    FuncNode(RET(*func)(Args...)) 
    : func(func) {
        desc = makeFuncNodeDesc(func);
        in_connections_ptrs.resize(desc.ins.size(), 0);
        in_connections.resize(desc.ins.size());
        out_buf.resize(desc.out_buf_sz);
        iterate_args(typename genseq<sizeof...(Args)>::type());
    }
    const FuncNodeDesc& getDesc() const override {
        return desc;
    } 

    void* getOutBuf() {
        return (void*)out_buf.data();
    }

    size_t inputCount() const override {
        return in_connections.size();
    }
    IFuncNode* getInputSource(size_t input_index, size_t& source_output_index) override {
        source_output_index = in_connections[input_index].src_out_index;
        return in_connections[input_index].src_node;
    }

    bool connectInput(size_t in_index, IFuncNode* node, size_t out_index) override {
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

        in_connections_ptrs[in_index] = (void*)((char*)node->getOutBuf() + out.buf_offset);
        in_connections[in_index].src_node = node;
        in_connections[in_index].src_out_index = out_index;

        return true;
    }

    void invoke() override {
        invoke_impl(typename genseq<sizeof...(Args)>::type());
    }
};

template<typename RET, typename... Args>
std::shared_ptr<IFuncNode> createFuncNode(RET(*func)(Args...)) {
    return std::shared_ptr<IFuncNode>(new FuncNode<RET, Args...>(func));
}



#endif
