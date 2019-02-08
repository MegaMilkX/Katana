#ifndef COLLISION_SENSOR_HPP
#define COLLISION_SENSOR_HPP

#include "../component.hpp"

class CollisionSensor : public Component {
    CLONEABLE_AUTO
    RTTR_ENABLE(Component)
public:


    virtual void serialize(std::ostream& out) {}
    virtual void deserialize(std::istream& in, size_t sz) {}
    virtual void _editorGui() {}
};
STATIC_RUN(CollisionSensor)
{
    rttr::registration::class_<CollisionSensor>("CollisionSensor")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif