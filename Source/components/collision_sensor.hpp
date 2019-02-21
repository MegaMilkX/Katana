#ifndef BOX_SENSOR_HPP
#define BOX_SENSOR_HPP

#include "../util/serialization.hpp"
#include <bulletcollision/collisiondispatch/btghostobject.h>

#include "base_collision_object.hpp"

#include "i_collision_sensor.hpp"

template<typename COLLISION_SHAPE_T>
class CollisionSensor 
: public CollisionObject<btGhostObject, COLLISION_SHAPE_T>,
public ICollisionSensor {
    CLONEABLE
    RTTR_ENABLE(CollisionObject<btGhostObject, COLLISION_SHAPE_T>)
public:
    CollisionSensor() {
        bt_object->setCollisionFlags(
            bt_object->getCollisionFlags() | 
            btCollisionObject::CF_HAS_CUSTOM_DEBUG_RENDERING_COLOR
        );
        bt_object->setCustomDebugColor(btVector3(1.0f, .15f, 0.25f));
    }
    ~CollisionSensor() {
        world->_removeSensor(this);
    }

    virtual void onCreate() {
        CollisionObject<btGhostObject, COLLISION_SHAPE_T>::onCreate();
        world->_addSensor(this);
    }

    virtual void update() {
        btGhostObject* ghost = (btGhostObject*)bt_object.get();

        std::set<SceneObject*> new_cache;
        for(int i = 0; i < ghost->getNumOverlappingObjects(); ++i) {
            btCollisionObject* object = ghost->getOverlappingObject(i);
            if(object->getInternalType() == btCollisionObject::CO_COLLISION_OBJECT) {
                continue;
            }

            SceneObject* so = (SceneObject*)object->getUserPointer();
            if(!object_cache.count(so)) {
                LOG("Entered: " << so->getName());
            }

            new_cache.insert(so);
        }

        std::swap(object_cache, new_cache);
    }
private:
    std::set<SceneObject*> object_cache;
};

typedef CollisionSensor<Collision::Box> BoxSensor_t;
typedef CollisionSensor<Collision::Sphere> SphereSensor_t;
typedef CollisionSensor<Collision::Cylinder> CylinderSensor_t;
typedef CollisionSensor<Collision::Capsule> CapsuleSensor_t;

STATIC_RUN(CollisionSensors)
{
    rttr::registration::class_<BoxSensor_t>("BoxSensor")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
    rttr::registration::class_<SphereSensor_t>("SphereSensor")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
    rttr::registration::class_<CylinderSensor_t>("CylinderSensor")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
    rttr::registration::class_<CapsuleSensor_t>("CapsuleSensor")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
