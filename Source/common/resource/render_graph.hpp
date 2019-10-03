#ifndef RENDER_GRAPH_HPP
#define RENDER_GRAPH_HPP

#include "resource.h"
#include "../util/func_graph/node_graph.hpp"

class RenderGraph : public Resource {
    RTTR_ENABLE(Resource)

public:
    JobGraph graph;

    void serialize(out_stream& out) {}
    virtual bool deserialize(in_stream& in, size_t sz) {
        return true;
    }
};
STATIC_RUN(RenderGraph) {
    rttr::registration::class_<RenderGraph>("RenderGraph")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
