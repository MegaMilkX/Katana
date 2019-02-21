#include "scene_physics_world.hpp"

btScalar ConvexSweepCallback::addSingleResult(btCollisionWorld::LocalConvexResult &convexResult, bool normalInWorldSpace) {
    if(convexResult.m_hitCollisionObject->getInternalType() == btCollisionObject::CO_GHOST_OBJECT) {
        return 0.0f;
    }
    // TODO: Check collision group mask here

    if(convexResult.m_hitFraction < closest_hit_fraction) {
        closest_hit_fraction = convexResult.m_hitFraction;
        gfxm::vec3 radius_vec = gfxm::normalize(to - from) * radius;
        closest_hit_center = gfxm::lerp(from, to, convexResult.m_hitFraction);
        closest_hit = closest_hit_center + radius_vec;
        has_hit = true;
    }

    gfxm::vec3 pos = gfxm::lerp(from, to, convexResult.m_hitFraction);
    /*
    world->getBtWorld()->getDebugDrawer()->drawSphere(
        radius,
        *(btTransform*)&gfxm::translate(gfxm::mat4(1.0f), pos),
        btVector3(0.6f, .3f, .0f)
    );*/

    return .0f;
}

void PhysicsWorld::update(float dt) {
    world->stepSimulation(dt);
    
    for(auto s : sensors) {
        s->update();
    }

/*
    int numManifolds = dispatcher->getNumManifolds();
    for(int i = 0; i < numManifolds; ++i) {
        btPersistentManifold* m = dispatcher->getMainifoldByIndexInternal(i);
        btCollisionObject* a = m->getBody0();
        btCollisionObject* b = m->getBody1();
        Collider* sensorCollider = 0;
        if(((Collider*)a->getUserPointer())->getType() == Collider::SENSOR) {
            sensorCollider = (Collider*)a->getUserPointer();
        } else if (((Collider*)b->getUserPointer())->getType() == Collider::SENSOR) {
            sensorCollider = (Collider*)b->getUserPointer();
        } else {
            continue;
        }

        
    }*/
}