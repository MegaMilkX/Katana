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