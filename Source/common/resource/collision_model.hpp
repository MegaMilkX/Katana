#ifndef COLLISION_MODEL_HPP
#define COLLISION_MODEL_HPP

#include "resource.h"
#include <btBulletCollisionCommon.h>


class CollisionModel : public Resource {
    RTTR_ENABLE(Resource)
public:
    std::string reference_model = "";

    void serialize(out_stream& out) override {
        
    }
    bool deserialize(in_stream& in, size_t sz) override {
        
        return true;
    }
    const char* getWriteExtension() const override { return "collision"; }
};
STATIC_RUN(CollisionModel) {
    rttr::registration::class_<CollisionModel>("CollisionModel")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}


#endif
