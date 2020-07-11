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

ecsEntityHandle ecsEntityHandle::getParent() const {
    return ecsEntityHandle(world, world->getParent(id));
}

void ecsEntityHandle::remove() {
    world->removeEntity(id);
    world = 0;
}

void ecsEntityHandle::removeTree() {
    world->removeTree(id);
    world = 0;
}

ecsEntityHandle ecsEntityHandle::findChild(const char* name) {
    return world->findChild(id, name);
}

uint64_t ecsEntityHandle::getAttribBitmask() {
    return world->getAttribBitmask(id);
}

uint64_t ecsEntityHandle::getInheritedAttribBitmask() {
    return world->getInheritedAttribBitmask(id);
}