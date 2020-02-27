#include "read_ctx.hpp"

#include "../attribute.hpp"
#include "../world.hpp"


ecsAttribBase* ecsWorldReadCtx::readAttribRef() {
    entity_id e = readEntityRef();
    if(e == ENTITY_ERROR) {
        return 0;
    }

    attrib_id attrib = read<uint16_t>();
    std::string attrib_name = getAttribName(attrib);

    attrib = getEcsAttribTypeLib().get_attrib_id(attrib_name.c_str());
    if(attrib < 0) {
        return 0;
    }

    ecsAttribBase* a = world->getAttribPtr(e, attrib);
    if(!a) {
        world->createAttrib(e, attrib);
        a = world->getAttribPtr(e, attrib);
    }

    return a;
}