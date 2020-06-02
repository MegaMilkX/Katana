#include "world.hpp"

#include "../util/data_writer.hpp"
#include "../util/data_reader.hpp"

#include "../resource/entity_template.hpp"
#include "attribs/base_attribs.hpp"


void ecsWorld::onAttribsCreated(entity_id ent, uint64_t entity_sig, uint64_t diff_sig) {
    // TODO ?
}
void ecsWorld::onAttribsRemoved(entity_id ent, uint64_t entity_sig, uint64_t diff_sig) {
    // TODO ?
}


void ecsWorld::setTemplateLink(entity_id e, std::shared_ptr<EntityTemplate> tpl) {
    clearTemplateLink(e);
    entity_to_template[e] = tpl;
    template_to_entities[tpl].insert(e);
    tpl->insertReferencingWorld(this);
}
void ecsWorld::clearTemplateLink(entity_id e) {
    auto it = entity_to_template.find(e);
    if(it != entity_to_template.end()) {
        if (template_to_entities[it->second].empty()) {
            it->second->eraseReferencingWorld(this);
        }

        template_to_entities[it->second].erase(e);
        entity_to_template.erase(e);
    }
}
void ecsWorld::tryClearTemplateLink(entity_id e) {
    if(entities.deref(e)->bitmaskInheritedAttribs == 0) {
        // Can clear all ties to the template
        clearTemplateLink(e);
    }
}


ecsWorld::ecsWorld() {
    global_attrib_counters.resize(get_last_attrib_id() + 1);
}

ecsWorld::~ecsWorld() {
    auto entity_set_copy = live_entities;
    for(auto& e : entity_set_copy) {
        removeEntity(e);
    }

    for(auto& kv : template_to_entities) {
        kv.first->eraseReferencingWorld(this);
    }
}

void ecsWorld::clearEntities (void) {
    auto le_copy = live_entities;
    for(auto e : le_copy) {
        removeEntity(e);
    }
}
void ecsWorld::clearSystems (void) {
    sys_by_type.clear();
    systems.clear();
}

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
ecsEntityHandle ecsWorld::createEntityFromTemplate (const char* tplPath) {
    auto tpl = retrieve<EntityTemplate>(tplPath);
    if(!tpl) {
        return ecsEntityHandle(0, -1);
    }

    ecsEntityHandle hdl = createEntity();
    dstream strm;
    tpl->serialize(strm);
    strm.jump(0);
    
    ecsWorldReadCtx ctx(this, &strm);
    ecsWorld::deserializeAttribDesc(ctx);
    ecsWorld::deserializeEntity(ctx, hdl.getId());

    uint64_t bitmaskInheritedAttribs = tpl->getEntity().getAttribBitmask();
    bitmaskInheritedAttribs &= ~(1 << getEcsAttribTypeLib().get_attrib_id("Translation"));
    bitmaskInheritedAttribs &= ~(1 << getEcsAttribTypeLib().get_attrib_id("Name"));

    // Mark all attributes as derived
    entities.deref(hdl.getId())->bitmaskInheritedAttribs = bitmaskInheritedAttribs;

    // Keep track of inherited entities
    setTemplateLink(hdl.getId(), tpl);
    auto translation = hdl.findAttrib<ecsTranslation>();
    if(translation) {
        // We don't need translation inheried from the template
        translation->setPosition(0,0,0);
    }

    return hdl;
}
void ecsWorld::removeEntity(entity_id id) {
    auto e = entities.deref(id);
    assert(e);

    for(auto& sys : systems) {
        // Signal this entity as an empty one
        sys->attribsRemoved(this, id, e->getAttribBits(), e->getAttribBits());
    }
    

    for (int i = 0; i < get_last_attrib_id() + 1; ++i) {
        if ((1 << i) & e->getAttribBits()) {
            global_attrib_counters[i]--;
            if (global_attrib_counters[i] == 0) {
                global_attrib_mask &= ~(1 << i);
            }
        }
    }

    clearTemplateLink(id);

    *e = ecsEntity(); // This removes attributes
    entities.free(id);
    live_entities.erase(id);
}
const std::set<entity_id>& ecsWorld::getEntities() const {
    return live_entities;
}
ecsEntityHandle ecsWorld::findEntity (const char* name) {
    for(auto e : live_entities) {
        auto n = findAttrib<ecsName>(e);
        if(!n) continue;
        if(n->name.compare(name) == 0) {
            return ecsEntityHandle(this, e);
        }
    }
    return ecsEntityHandle(0, -1);
}
void ecsWorld::setParent (entity_id parent, entity_id child) {
    auto e = entities.deref(child);
    e->parent_uid = parent;
    // TODO: Signal systems
}


void ecsWorld::createAttrib(entity_id ent, attrib_id attrib) {
    auto e = entities.deref(ent);
    auto a = e->getAttrib(attrib);

    global_attrib_counters[attrib]++;
    global_attrib_mask |= (1 << attrib);

    for(auto& sys : systems) {
        sys->attribsCreated(this, ent, e->getAttribBits(), 1 << attrib);
    }
}
void ecsWorld::setAttribInheritanceMask(entity_id e, uint64_t mask) {
    auto ent = entities.deref(e);
    assert(ent);
    ent->bitmaskInheritedAttribs = mask;
}
void ecsWorld::clearAttribInheritance(entity_id e, attrib_id attrib) {
    auto ent = entities.deref(e);
    assert(ent);
    if(ent->bitmaskInheritedAttribs & (1 << attrib)) {
        ent->bitmaskInheritedAttribs &= ~(1 << attrib);
    }
    tryClearTemplateLink(e);
}
void ecsWorld::clearAllAttribInheritance(entity_id e) {
    auto ent = entities.deref(e);
    assert(ent);
    ent->bitmaskInheritedAttribs = 0;
    clearTemplateLink(e);
}
void ecsWorld::removeAttrib(entity_id ent, attrib_id attrib) {
    auto e = entities.deref(ent);
    for(auto& sys : systems) {
        sys->attribsRemoved(this, ent, e->getAttribBits(), 1 << attrib);
    }

    global_attrib_counters[attrib]--;
    if(global_attrib_counters[attrib] == 0) {
        global_attrib_mask &= ~(1 << attrib);
    }

    e->removeAttrib(attrib);
}
void ecsWorld::removeAttribs(entity_id ent, uint64_t mask) {
    for(int i = 0; i < get_last_attrib_id() + 1; ++i) {
        if(mask & (1 << i)) {
            removeAttrib(ent, i);
        }
    }
}


ecsAttribBase* ecsWorld::getAttribPtr(entity_id ent, attrib_id id) {
    auto e = entities.deref(ent);
    if(!e) return 0;
    return e->getAttribPtr(id);
}

void ecsWorld::copyAttribs (entity_id dst, ecsEntityHandle src, uint64_t ignore_mask) {
    dstream strm;
    ecsWorldWriteCtx wctx(src.getWorld(), &strm);
    ecsWorld::serializeAttribDesc(wctx);
    ecsWorld::serializeEntity(wctx, src.getId());
    strm.jump(0);

    ecsWorldReadCtx rctx(this, &strm);
    ecsWorld::deserializeAttribDesc(rctx);
    ecsWorld::deserializeEntity(rctx, dst, ignore_mask);
}

uint64_t ecsWorld::getAttribBitmask(entity_id e) {
    auto ent = entities.deref(e);
    if(!ent) {
        assert(false);
        return 0;
    }
    return ent->attrib_bits;
}

uint64_t ecsWorld::getInheritedAttribBitmask(entity_id e) {
    auto ent = entities.deref(e);
    if(!ent) {
        assert(false);
        return 0;
    }
    return ent->bitmaskInheritedAttribs;
}

EntityTemplate* ecsWorld::updateTemplate(entity_id ent) {
    auto it = entity_to_template.find(ent);
    if(it == entity_to_template.end()) {
        LOG_WARN("Attempted to update template of an entity without template");
        return 0;
    }

    it->second->update(ecsEntityHandle(this, ent));

    it->second->updateDerivedEntities();

    return it->second.get();
}

void ecsWorld::updateDerived(std::shared_ptr<EntityTemplate> tpl) {
    dstream strm;
    tpl->serialize(strm);

    auto it2 = template_to_entities.find(tpl);
    for(auto e : it2->second) {
        strm.jump(0);

        uint64_t ignore_mask = ~getInheritedAttribBitmask(e);

        ecsWorldReadCtx rctx(this, &strm);
        ecsWorld::deserializeAttribDesc(rctx);
        ecsWorld::deserializeEntity(rctx, e, ignore_mask);
    }
}

void ecsWorld::linkToTemplate(entity_id ent, std::shared_ptr<EntityTemplate> tpl) {
    uint64_t ignore_mask = 0;
    ignore_mask |= (1 << getEcsAttribTypeLib().get_attrib_id("Translation"));
    ignore_mask |= (1 << getEcsAttribTypeLib().get_attrib_id("Name"));
    copyAttribs(ent, tpl->getEntity(), ignore_mask);
    uint64_t inheritanceMask = tpl->getEntity().getAttribBitmask();
    inheritanceMask &= ~ignore_mask;
    setAttribInheritanceMask(ent, inheritanceMask);
    setTemplateLink(ent, tpl);
}

void ecsWorld::resetToTemplate(entity_id ent) {
    auto it = entity_to_template.find(ent);
    if(it == entity_to_template.end()) {
        LOG_WARN("Attempted to reset an entity to template without template link");
        return;
    }

    // Do not clear Translation and Name
    uint64_t remove_mask = 0xffffffffffffffff;
    remove_mask &= ~(1 << getEcsAttribTypeLib().get_attrib_id("Translation"));
    remove_mask &= ~(1 << getEcsAttribTypeLib().get_attrib_id("Name"));
    uint64_t ignore_mask = ~remove_mask;
    
    removeAttribs(ent, remove_mask);
    copyAttribs(ent, it->second->getEntity(), ignore_mask);
    setAttribInheritanceMask(ent, it->second->getEntity().getAttribBitmask() & ~ignore_mask);
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

int ecsWorld::systemCount() {
    return (int)systems.size();
}
ecsSystemBase*  ecsWorld::getSystem(int i) {
    return systems[i].get();
}

void ecsWorld::update() {
    timer t;
    t.start();

    for(auto& kv : storages) {
        kv.second->onUpdateBegin();
    }

    for(auto& sys : systems) {
        sys->onUpdate();
    }

    for(auto& kv : storages) {
        kv.second->onUpdateEnd();
    }

    //LOG("ELAPSED: " << t.stop());
}


#include "util/write_ctx.hpp"
#include "util/read_ctx.hpp"

void ecsWorld::serialize(out_stream& out) {
    ecsWorldWriteCtx ctx(this, &out);

    serializeAttribDesc(ctx);

    entity_id next_new_id = 0;
    for(auto it = live_entities.begin(); it != live_entities.end(); ++it) {
        ctx.remapEntityId((*it), next_new_id++);
    }

    // Save template references
    ctx.write<uint32_t>(entity_to_template.size());
    for(auto& p : entity_to_template) {
        ctx.write<uint64_t>(ctx.getRemappedEntityId(p.first));
        ctx.writeResource(p.second);
    }

    // Entities
    ctx.write<uint64_t>(live_entities.size());
    for(auto& e : live_entities) {
        serializeEntity(ctx, e, true);
    }
}
bool ecsWorld::deserialize(in_stream& in, size_t sz) {
    ecsWorldReadCtx ctx(this, &in);

    deserializeAttribDesc(ctx);

    // Template references
    uint32_t template_ref_count = ctx.read<uint32_t>();
    for(int i = 0; i < template_ref_count; ++i) {
        entity_id e = ctx.read<uint64_t>();
        auto tpl = ctx.readResource<EntityTemplate>();
        setTemplateLink(e, tpl);
    }

    // Entities
    uint64_t ent_count = ctx.read<uint64_t>();
    for(int i = 0; i < ent_count; ++i) {
        createEntity();
    }

    for(int i = 0; i < ent_count; ++i) {
        deserializeEntity(ctx, i);
    }

    return true;
}


void ecsWorld::serializeAttribDesc (ecsWorldWriteCtx& ctx) {
    uint16_t known_attrib_count = get_last_attrib_id() + 1;
    ctx.write<uint16_t>(known_attrib_count);
    for(int i = 0; i <= get_last_attrib_id(); ++i) {
        auto inf = getEcsAttribTypeLib().get_info(i);
        std::string attrib_name;
        if(inf) {
            attrib_name = inf->name;
        } else {
            LOG_ERR("Failed to get inf for attrib " << i);
            assert(false);
        }
        ctx.writeStr(attrib_name);
    }
}
void ecsWorld::deserializeAttribDesc (ecsWorldReadCtx& ctx) {
    uint16_t known_attrib_count = ctx.read<uint16_t>();
    for(uint16_t i = 0; i < known_attrib_count; ++i) {
        std::string attrib_name = ctx.readStr();
        ctx.setAttribName(i, attrib_name.c_str());
    }
}

static int countSetBits(uint64_t i)
{
    int count = 0;
    for (; i; i >>= 1) {
      count += (i & 1);
    }
    return count;
}

void ecsWorld::serializeEntity (ecsWorldWriteCtx& ctx, entity_id e, bool keep_template_link) {
    auto ent = ctx.getWorld()->entities.deref(e);

    uint32_t attrib_count = 0;
    uint64_t inheritedBitmask = ctx.getWorld()->getInheritedAttribBitmask(e);
    if(keep_template_link) {
        attrib_count = countSetBits(ent->getAttribBits() & ~inheritedBitmask);
    } else {
        attrib_count = countSetBits(ent->getAttribBits());
    }

    ctx.write<uint64_t>(inheritedBitmask);
    ctx.write<uint32_t>(attrib_count);
    for(int i = 0; i <= get_last_attrib_id(); ++i) {
        if(keep_template_link && (inheritedBitmask & (1 << i))) {
            continue;
        }
        ecsAttribBase* a = ent->findAttrib(i);
        if(!a) {
            continue;
        }

        ctx.write<uint16_t>(i);
        
        ctx.beginSubBlock();
        
        a->write(ctx);
        
        ctx.endSubBlock();
    }
}
void ecsWorld::deserializeEntity (ecsWorldReadCtx& ctx, entity_id e, uint64_t attrib_ignore_mask) {
    uint64_t inheritedBitmask = ctx.read<uint64_t>();
    inheritedBitmask = ctx.convertMask(inheritedBitmask);
    uint32_t attrib_count = ctx.read<uint32_t>();

    if(inheritedBitmask) {
        auto it = ctx.getWorld()->entity_to_template.find(e);
        if(it != ctx.getWorld()->entity_to_template.end()) {
            ctx.getWorld()->copyAttribs(e, it->second->getEntity(), ~inheritedBitmask);
            ctx.getWorld()->setAttribInheritanceMask(e, inheritedBitmask);
        }
    }

    for(int j = 0; j < attrib_count; ++j) {
        attrib_id attrib_index = (attrib_id)ctx.read<uint16_t>();

        uint64_t attrib_byte_count = ctx.read<uint64_t>(); // Byte count saved by endSubBlock()
        
        auto& attrib_name = ctx.getAttribName(attrib_index);
        // TODO: if attrib name is "" or other error - skip bytes
        if (attrib_name == "WorldTransform") {
          //__debugbreak();
        }

        attrib_index = getEcsAttribTypeLib().get_attrib_id(attrib_name.c_str());

        ecsAttribBase* a = ctx.getWorld()->getAttribPtr(e, attrib_index);

        if(attrib_ignore_mask & (1 << attrib_index)) {
            ctx.skipBytes(attrib_byte_count);
            continue;
        }
        
        if(!a) {
            ctx.getWorld()->createAttrib(e, attrib_index);
            a = ctx.getWorld()->getAttribPtr(e, attrib_index);
        }
        if(a) {
            a->read(ctx);
            ctx.getWorld()->signalAttribUpdate(e, attrib_index);
        }
    }
}
