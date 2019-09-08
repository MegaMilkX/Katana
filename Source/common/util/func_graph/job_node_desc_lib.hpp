#ifndef JOB_NODE_DESC_LIB_HPP
#define JOB_NODE_DESC_LIB_HPP

#include <memory>
#include <map>
#include <string>

#include <rttr/type>

#include "func_node_desc.hpp"

#include "../singleton.hpp"


class JobNodeDescLib : public Singleton<JobNodeDescLib> {
    std::map<
        std::string,
        std::unique_ptr<FuncNodeDesc>
    > descs;
    std::map<
        rttr::type,
        FuncNodeDesc*
    > descs_by_type;
public:
    FuncNodeDesc* getDesc(const std::string& name) {
        auto it = descs.find(name);
        if(it == descs.end()) {
            return 0;
        } else {
            return it->second.get();
        }
    }
    FuncNodeDesc* getDesc(rttr::type t) {
        auto it = descs_by_type.find(t);
        if(it == descs_by_type.end()) {
            return 0;
        } else {
            return it->second;
        }
    }

    FuncNodeDesc* createDesc(rttr::type t, const std::string& name) {
        FuncNodeDesc* ptr = 0;
        auto it = descs.find(name);
        if(it != descs.end()) {
            return 0;
        }
        
        ptr = new FuncNodeDesc;
        ptr->name = name;
        descs[name].reset(ptr);
        descs_by_type[t] = ptr;
        return ptr;
    }
};

inline JobGraphNode* createJobNode(const std::string& name) {
    auto desc = JobNodeDescLib::get()->getDesc(name);
    return desc->node_constructor();
}

template<typename T>
class regJobNode {
    FuncNodeDesc* desc = 0;
public:
    regJobNode(const std::string& name) {
        desc = JobNodeDescLib::get()->createDesc(rttr::type::get<T>(), name);
        desc->node_constructor = []()->JobGraphNode*{
            return new T();
        };
    }
    ~regJobNode() {

    }

    template<typename T>
    regJobNode& in(const std::string& name, unsigned int count = 1) {
        for(unsigned int i = 0; i < count; ++i) {
            desc->ins.emplace_back(FuncNodeDesc::In{ name, rttr::type::get<T>() });
        }
        return *this;
    }
    template<typename T>
    regJobNode& out(const std::string& name, int count = 1) {
        for(unsigned int i = 0; i < count; ++i) {
            desc->outs.emplace_back(FuncNodeDesc::Out{ name, rttr::type::get<T>() });
        }
        return *this;
    }
    regJobNode& color(float r, float g, float b) {
        desc->color = gfxm::vec3(r, g, b);
        return *this;
    }
};

#endif
