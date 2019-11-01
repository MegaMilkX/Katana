#include "world.hpp"


entity_id ecsWorld::createEntity() {
    entity_id id = entities.acquire();
    live_entities.insert(id);
    return id;
}
entity_id ecsWorld::createEntity(archetype_mask_t attrib_signature) {
    entity_id ent = createEntity();
    for(int i = 0; i < 64; ++i) {
        if(attrib_signature & (1 << i)) {
            setAttrib(ent, i);
        }
    }
    return ent;
}
void ecsWorld::removeEntity(entity_id id) {
    for(auto& sys : systems) {
        // Signal this entity as an empty one
        sys->tryFit(this, id, 0);
    }

    *entities.deref(id) = ecsEntity();
    entities.free(id);
    live_entities.erase(id);
}
ecsEntity* ecsWorld::getEntity(entity_id id) {
    return entities.deref(id);
}

const std::set<entity_id>& ecsWorld::getEntities() const {
    return live_entities;
}


void ecsWorld::setAttrib(entity_id ent, attrib_id attrib) {
    auto e = entities.deref(ent);
    auto a = e->getAttrib(attrib);
    for(auto& sys : systems) {
        sys->tryFit(this, ent, e->getAttribBits());
    }
}
void ecsWorld::removeAttrib(entity_id ent, attrib_id attrib) {
    auto e = entities.deref(ent);
    e->removeAttrib(attrib);
    for(auto& sys : systems) {
        sys->tryFit(this, ent, e->getAttribBits());
    }
}


ecsAttribBase* ecsWorld::getAttribPtr(entity_id ent, attrib_id id) {
    auto e = entities.deref(ent);
    if(!e) return 0;
    return e->getAttribPtr(id);
}


void ecsWorld::addSystems(const std::vector<ecsSystemBase*>& sys_array) {
    for(auto& sys : sys_array) {
        sys->world = this;
        systems.push_back(std::unique_ptr<ecsSystemBase>(sys));
    }
}

void ecsWorld::update() {
    timer t;
    t.start();
    for(auto& sys : systems) {
        sys->onUpdate();
    }
    LOG("ELAPSED: " << t.stop());
}