#include "world.hpp"


ecsEntityHandle ecsWorld::createEntity() {
    entity_id id = entities.acquire();
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


void ecsWorld::onGuiNodeTree() {
    
}