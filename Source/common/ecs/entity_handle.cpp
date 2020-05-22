#include "entity_handle.hpp"

#include "world.hpp"

bool ecsEntityHandle::isValid() const {
    // TODO
    return world != 0;
}

entity_id ecsEntityHandle::getId() const {
    return id;
}
ecsWorld* ecsEntityHandle::getWorld() {
    return world;
}

void ecsEntityHandle::remove() {
    world->removeEntity(id);
    world = 0;
}

uint64_t ecsEntityHandle::getAttribBitmask() {
    return world->getAttribBitmask(id);
}

uint64_t ecsEntityHandle::getInheritedAttribBitmask() {
    return world->getInheritedAttribBitmask(id);
}