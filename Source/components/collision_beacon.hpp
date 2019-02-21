#ifndef COLLISION_BEACON_HPP
#define COLLISION_BEACON_HPP

#include "base_collision_object.hpp"

template<typename COLLISION_SHAPE_T>
class CollisionBeacon
: public CollisionObject<btGhostObject, COLLISION_SHAPE_T> {
    CLONEABLE
    RTTR_ENABLE(CollisionObject<btGhostObject, COLLISION_SHAPE_T>)
public:
    CollisionBeacon() {
        bt_object->setCollisionFlags(
            bt_object->getCollisionFlags() | 
            btCollisionObject::CF_HAS_CUSTOM_DEBUG_RENDERING_COLOR
        );
        bt_object->setCustomDebugColor(btVector3(1.0f, .4f, 0.15f));
    }
};

typedef CollisionBeacon<Collision::Box> BoxBeacon;
typedef CollisionBeacon<Collision::Sphere> SphereBeacon;
typedef CollisionBeacon<Collision::Cylinder> CylinderBeacon;
typedef CollisionBeacon<Collision::Capsule> CapsuleBeacon;

STATIC_RUN(CollisionBeacons)
{
    rttr::registration::class_<BoxBeacon>("BoxBeacon")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
    rttr::registration::class_<SphereBeacon>("SphereBeacon")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
    rttr::registration::class_<CylinderBeacon>("CylinderBeacon")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
    rttr::registration::class_<CapsuleBeacon>("CapsuleBeacon")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
