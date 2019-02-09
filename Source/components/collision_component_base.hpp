#ifndef COLLISION_COMPONENT_BASE_HPP
#define COLLISION_COMPONENT_BASE_HPP

#include "../component.hpp"

class BaseCollisionComponent : public Component {
    RTTR_ENABLE(Component)
public:
    virtual void refreshShape() = 0;
};

#endif
