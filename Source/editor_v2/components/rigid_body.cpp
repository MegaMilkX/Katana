#include "rigid_body.hpp"

STATIC_RUN(CmRigidBody) {
    rttr::registration::class_<CmRigidBody>("CmRigidBody")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}