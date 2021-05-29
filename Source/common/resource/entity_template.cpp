#include "entity_template.hpp"

#include "../ecs/world.hpp"


static ecsWorld& getTemplateCacheWorld() {
    static ecsWorld templateCacheWorld;
    return templateCacheWorld;
}


EntityTemplate::EntityTemplate() {
    hdl = getTemplateCacheWorld().createEntity();
}

ecsEntityHandle EntityTemplate::getEntity() {
    return hdl;
}

void EntityTemplate::insertReferencingWorld(ecsWorld* wrld) {
    referencing_worlds.insert(wrld);
}
void EntityTemplate::eraseReferencingWorld(ecsWorld* wrld) {
    referencing_worlds.erase(wrld);
}

void EntityTemplate::updateDerivedEntities() {
    for(auto w : referencing_worlds) {
        w->updateDerived(this->shared_from_this());
    }
}

void EntityTemplate::update(ecsEntityHandle hdl) {
    dstream strm;
    ecsWorldWriteCtx wctx(hdl.getWorld(), &strm);
    ecsWorld::serializeAttribDesc(wctx);
    ecsWorld::serializeEntity(wctx, hdl.getId());

    strm.jump(0);

    uint64_t ignore_mask = 0;
    ignore_mask |= (1ULL << getEcsAttribTypeLib().get_attrib_id("Translation")); // Do not store translation
    uint64_t name_mask = (1ULL << getEcsAttribTypeLib().get_attrib_id("Name"));

    ecsWorldReadCtx rctx(&getTemplateCacheWorld(), &strm);
    ecsWorld::deserializeAttribDesc(rctx);
    ecsWorld::deserializeEntity(rctx, this->hdl.getId(), ignore_mask);

    uint64_t inheritance_mask = this->hdl.getAttribBitmask();
    inheritance_mask &= ~ignore_mask;
    inheritance_mask &= ~name_mask;

    hdl.getWorld()->setAttribInheritanceMask(hdl.getId(), inheritance_mask);
}

void EntityTemplate::serialize(out_stream& out) {
    ecsWorldWriteCtx ctx(&getTemplateCacheWorld(), &out);
    ecsWorld::serializeAttribDesc(ctx);
    ecsWorld::serializeEntity(ctx, hdl.getId());
}

bool EntityTemplate::deserialize(in_stream& in, size_t sz) {
    ecsWorldReadCtx ctx(&getTemplateCacheWorld(), &in);
    ecsWorld::deserializeAttribDesc(ctx);
    ecsWorld::deserializeEntity(ctx, hdl.getId());
    return true;
}