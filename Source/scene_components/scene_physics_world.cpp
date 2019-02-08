#include "scene_physics_world.hpp"

#include "../components/collider.hpp"

btScalar ConvexSweepCallback::addSingleResult(btCollisionWorld::LocalConvexResult &convexResult, bool normalInWorldSpace) {
    Collider* col = (Collider*)convexResult.m_hitCollisionObject->getUserPointer();
    if((col->getGroupMask() & flags) == 0) {
        return 0.0f;
    }

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