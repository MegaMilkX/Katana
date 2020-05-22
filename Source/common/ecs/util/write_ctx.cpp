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

void ecsWorldWriteCtx::beginSubBlock() {
    subblock_stream.jump(0);
    subblock_stream.setBuffer(std::vector<char>());
    is_in_subblock = true;
    current_stream = &subblock_stream;
}
void ecsWorldWriteCtx::endSubBlock() {
    is_in_subblock = false;
    current_stream = strm;
    auto& buf = subblock_stream.getBuffer();
    write<uint64_t>(buf.size());
    strm->write(buf.data(), buf.size());
}