#include "read_ctx.hpp"

#include "../attribute.hpp"
#include "../world.hpp"


ecsAttribBase* ecsWorldReadCtx::readAttribRef() {
    entity_id e = readEntityRef();
    if(e == ENTITY_ERROR) {
        return 0;
    }
    std::string attrib_name = readStr();

    attrib_id attr = getEcsAttribTypeLib().get_attrib_id(attrib_name.c_str());
    if(attr < 0) {
        return 0;
    }

    ecsAttribBase* a = world->getAttribPtr(e, attr);
    if(!a) {
        world->createAttrib(e, attr);
        a = world->getAttribPtr(e, attr);
    }

    return a;
}