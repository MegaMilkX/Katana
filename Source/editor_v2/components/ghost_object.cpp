#include "ghost_object.hpp"

STATIC_RUN(GhostObject) {
    rttr::registration::class_<GhostObject>("GhostObject")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}