#ifndef ECS_WORLD_HPP
#define ECS_WORLD_HPP

#include "resource.h"

class EcsWorld : public Resource {
    RTTR_ENABLE(Resource)
public:
    virtual void serialize(out_stream& out) {

    }
    virtual bool deserialize(in_stream& in, size_t sz) {
        return true;
    }
};
STATIC_RUN(EcsWorld) {
    rttr::registration::class_<EcsWorld>("EcsWorld")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
