#ifndef BLEND_TREE_HPP
#define BLEND_TREE_HPP

#include "resource.h"

class BlendTree : public Resource {
    RTTR_ENABLE(Resource)
    
    
public:
    const char* getWriteExtension() const override { return "blend_tree"; }

    
};
STATIC_RUN(BlendTree) {
    rttr::registration::class_<BlendTree>("BlendTree")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
