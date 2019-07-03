#include "tiled_room.hpp"

STATIC_RUN(REG_ATTRIB) {
    rttr::registration::class_<attribTiledRoomCtrl>("TiledRoomCtrl")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );

    // TODO: Move attribute reg here
}