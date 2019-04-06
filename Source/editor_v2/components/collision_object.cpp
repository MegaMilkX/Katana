#include "collision_object.hpp"

STATIC_RUN(CmCollisionObject) {
    rttr::registration::class_<CmCollisionObject>("CmCollisionObject")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}