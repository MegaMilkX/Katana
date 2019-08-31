#ifndef FUNC_NODE_DESC_HPP
#define FUNC_NODE_DESC_HPP

#include "arg_info.hpp"

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

#endif
