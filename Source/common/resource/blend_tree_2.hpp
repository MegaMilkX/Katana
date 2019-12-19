#ifndef BLEND_TREE_2_HPP
#define BLEND_TREE_2_HPP

#include "resource.h"

class BlendTree2 : public Resource {
    RTTR_ENABLE(Resource)
public:
};
STATIC_RUN(BlendTree2) {
    rttr::registration::class_<BlendTree2>("BlendTree2")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
