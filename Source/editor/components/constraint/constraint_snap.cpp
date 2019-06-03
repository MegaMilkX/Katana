#include "constraint_snap.hpp"

namespace Constraint {

STATIC_RUN(Snap) {
    rttr::registration::class_<Snap>("SnapConstraint")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

}