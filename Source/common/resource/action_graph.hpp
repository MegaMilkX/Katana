#ifndef ACTION_GRAPH_HPP
#define ACTION_GRAPH_HPP

#include "resource.h"

struct ActionGraphTransition {
    float blendTime = 0.1f;
};

class ActionGraphNode {
    
};

class ActionGraphLayer {
public:
};

// ==========================

class ActionGraph : public Resource {
    RTTR_ENABLE(Resource)
public:
    const char* getWriteExtension() const override { return "action_graph"; }


};
STATIC_RUN(ActionGraph) {
    rttr::registration::class_<ActionGraph>("ActionGraph")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
