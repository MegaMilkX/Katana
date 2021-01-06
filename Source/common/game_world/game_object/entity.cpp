#include "entity.hpp"

#include "../game_world.hpp"

ktEntity::ktEntity(ktGameWorld* world)
: world(world) {
    archetype = world->getArchetypeGraph()->getNullNode();
}