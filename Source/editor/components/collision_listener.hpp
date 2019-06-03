#ifndef COLLISION_TRIGGER_HPP
#define COLLISION_TRIGGER_HPP

#include "component.hpp"

class CollisionListener : public Attribute {
    RTTR_ENABLE(Attribute)
public:
    virtual void onEnter(GameObject* other) = 0;
    virtual void onLeave(GameObject* other) = 0;
};
//REG_ATTRIB(CollisionListener);

#endif
