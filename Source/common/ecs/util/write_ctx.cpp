#include "write_ctx.hpp"

#include "../attribute.hpp"


void ecsWorldWriteCtx::writeAttribRef(ecsAttribBase* attrib) {
    if(!attrib) {
        write<uint64_t>(SERIALIZED_ENTITY_ERROR);
        return;
    }

    write<uint64_t>(attrib->getEntityId());
    auto inf = getEcsAttribTypeLib().get_info(attrib->get_id());

    write<uint16_t>(attrib->get_id());
}