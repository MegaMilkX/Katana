#include "collider.hpp"

#include "../scene.hpp"

void Collider::onCreate() {
    world = getScene()->getSceneComponent<PhysicsWorld>();
    world->getBtWorld()->addCollisionObject(collision_object.get());

    transform = getObject()->get<Transform>();
    updateTransform();
}