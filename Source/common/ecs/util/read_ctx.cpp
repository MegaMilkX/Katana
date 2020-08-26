#include "read_ctx.hpp"

#include "../attribute.hpp"
#include "../world.hpp"


entity_id ecsWorldReadCtx::readAttribRef() {
    entity_id e = readEntityRef();
    if(e == ENTITY_ERROR) {
        return 0;
    }

    attrib_id attrib = read<uint16_t>();
    std::string attrib_name = getAttribName(attrib);

    attrib = getEcsAttribTypeLib().get_attrib_id(attrib_name.c_str());
    if(attrib < 0) {
        return ENTITY_ERROR;
    }

    return e;
}