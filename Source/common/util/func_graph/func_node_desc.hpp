#ifndef FUNC_NODE_DESC_HPP
#define FUNC_NODE_DESC_HPP

#include <functional>

#include "arg_info.hpp"

#include "job_graph_node.hpp"

#include "../common/gfxm.hpp"

struct FuncNodeDesc {
    std::function<JobGraphNode*(void)> node_constructor;
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
    gfxm::vec3 color;
};

#endif
