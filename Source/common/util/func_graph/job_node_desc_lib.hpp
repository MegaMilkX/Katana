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
public:
    FuncNodeDesc* getDesc(const std::string& name) {
        FuncNodeDesc* ptr = 0;
        auto it = descs.find(name);
        if(it == descs.end()) {
            ptr = new FuncNodeDesc();
            descs.insert(std::make_pair(name, std::unique_ptr<FuncNodeDesc>(ptr)));
        } else {
            ptr = it->second.get();
        }

        return ptr;
    }    
};

template<typename T>
class regJobNode {
    FuncNodeDesc* desc = 0;
public:
    regJobNode(const std::string& name) {
        desc = JobNodeDescLib::get()->getDesc(rttr::type::get<T>().get_name().to_string());
        desc->name = name;
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
};

#endif
