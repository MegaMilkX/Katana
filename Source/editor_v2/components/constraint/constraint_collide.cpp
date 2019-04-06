#include "constraint_collide.hpp"

namespace Constraint {

STATIC_RUN(Collide) {
    rttr::registration::class_<Collide>("CollideConstraint")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

}