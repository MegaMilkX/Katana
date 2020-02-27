#include "world.hpp"

#include "../util/data_writer.hpp"
#include "../util/data_reader.hpp"


ecsEntityHandle ecsWorld::createEntity() {
    entity_id id = entities.acquire();
    entities.deref(id)->entity_uid = id;
    live_entities.insert(id);
    return ecsEntityHandle(this, id);
}
ecsEntityHandle ecsWorld::createEntity(archetype_mask_t attrib_signature) {
    auto hdl = createEntity();
    for(int i = 0; i < 64; ++i) {
        if(attrib_signature & (1 << i)) {
            createAttrib(hdl.getId(), i);
        }
    }
    return hdl;
}
void ecsWorld::removeEntity(entity_id id) {
    for(auto& sys : systems) {
        // Signal this entity as an empty one
        auto e = entities.deref(id);
        sys->attribsRemoved(this, id, e->getAttribBits(), 0);
    }

    *entities.deref(id) = ecsEntity();
    entities.free(id);
    live_entities.erase(id);
}


const std::set<entity_id>& ecsWorld::getEntities() const {
    return live_entities;
}


void ecsWorld::createAttrib(entity_id ent, attrib_id attrib) {
    auto e = entities.deref(ent);
    auto a = e->getAttrib(attrib);
    for(auto& sys : systems) {
        sys->attribsCreated(this, ent, e->getAttribBits(), 1 << attrib);
    }
}
void ecsWorld::removeAttrib(entity_id ent, attrib_id attrib) {
    auto e = entities.deref(ent);
    for(auto& sys : systems) {
        sys->attribsRemoved(this, ent, e->getAttribBits(), 1 << attrib);
    }
    e->removeAttrib(attrib);
}


ecsAttribBase* ecsWorld::getAttribPtr(entity_id ent, attrib_id id) {
    auto e = entities.deref(ent);
    if(!e) return 0;
    return e->getAttribPtr(id);
}


void ecsWorld::signalAttribUpdate(entity_id ent, attrib_id attrib) {
    uint64_t attr_mask = 1 << attrib;
    auto e = entities.deref(ent);
    if(e->getAttribBits() & attr_mask) {
        for(auto& sys : systems) {
            sys->signalUpdate(ent, attr_mask);
        }
    }
}


void ecsWorld::update() {
    timer t;
    t.start();
    for(auto& sys : systems) {
        sys->onUpdate();
    }
    //LOG("ELAPSED: " << t.stop());
}


static int countSetBits(uint64_t i)
{
    int count = 0;
    for(;i;i>>=1) {
        count += (i & 1);
    }
    return count;
}

#include "util/write_ctx.hpp"
#include "util/read_ctx.hpp"

void ecsWorld::serialize(out_stream& out) {
    ecsWorldWriteCtx ctx(this, &out);

    entity_id next_new_id = 0;
    for(auto it = live_entities.begin(); it != live_entities.end(); ++it) {
        ctx.remapEntityId((*it), next_new_id++);
    }

    ctx.write<uint64_t>(live_entities.size());
    for(auto& e : live_entities) {
        auto ent = entities.deref(e);

        ctx.write<uint32_t>(countSetBits(ent->getAttribBits()));
        for(int i = 0; i <= get_last_attrib_id(); ++i) {
            ecsAttribBase* a = ent->findAttrib(i);
            if(!a) {
                continue;
            }

            ctx.write<uint16_t>(i);
            a->write(ctx);
        }
    }
}
bool ecsWorld::deserialize(in_stream& in, size_t sz) {
    ecsWorldReadCtx ctx(this, &in);

    uint64_t ent_count = ctx.read<uint64_t>();
    for(int i = 0; i < ent_count; ++i) {
        createEntity();
    }

    for(int i = 0; i < ent_count; ++i) {
        uint32_t attrib_count = ctx.read<uint32_t>();
        for(int j = 0; j < attrib_count; ++j) {
            attrib_id attrib_index = (attrib_id)ctx.read<uint16_t>();
            ecsAttribBase* a = getAttribPtr(i, attrib_index);
            if(!a) {
                createAttrib(i, attrib_index);
                a = getAttribPtr(i, attrib_index);
            }
            if(a) {
                a->read(ctx);
            }
        }
    }

    /*
    DataReader r(&in);

    uint64_t ent_count = r.read<uint64_t>();
    for(uint64_t i = 0; i < ent_count; ++i) {
        auto hdl = createEntity();
        
        uint32_t attrib_count = r.read<uint32_t>();
        for(uint32_t j = 0; j < attrib_count; ++j) {
            //std::string attrib_name = r.readStr();
            uint64_t attrib_id = r.read<uint64_t>();
            createAttrib(hdl.getId(), attrib_id);
            auto ptr = getAttribPtr(hdl.getId(), attrib_id);
            ptr->read(in);
        }
    }*/

    return true;
}