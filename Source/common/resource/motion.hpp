#ifndef MOTION_HPP
#define MOTION_HPP

#include "resource.h"
#include "anim_primitive.hpp"

#include <memory>

class Motion : public Resource {
    RTTR_ENABLE(Resource)

    std::unique_ptr<AnimPrimitive> primitive;
    // TODO: Parameter buffer here

public:
    const char* getWriteExtension() const override { return "motion"; }

};
STATIC_RUN(Motion) {
    rttr::registration::class_<Motion>("Motion")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
