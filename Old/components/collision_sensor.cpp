#include "collision_sensor.hpp"

#include "../scene.hpp"

void CollisionSensor::onCreate() {
    setShape<CollisionBox>();

    world = getScene()->getSceneComponent<PhysicsWorld>();
    world->getBtWorld()->addCollisionObject(ghost_object.get());

    transform = getObject()->get<Transform>();
    updateTransform();
}