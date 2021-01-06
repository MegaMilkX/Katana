#include "actor.hpp"

#include "../game_world.hpp"


ktCharacter::ktCharacter(ktGameWorld* world)
: ktGameObject(world) {

    collision_hull.reset(new btCollisionObject());
    btCapsuleShape* capsule = new btCapsuleShape(0.5f, 2.0f);
    collision_hull->setCollisionShape(capsule);
    hull_shape.reset(capsule);
  
    world->getDynamicsWorld()->getBtWorld()->addCollisionObject(collision_hull.get());
}
ktCharacter::~ktCharacter() {
    getWorld()->getDynamicsWorld()->getBtWorld()->removeCollisionObject(collision_hull.get());
}