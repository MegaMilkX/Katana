#ifndef I_COLLISION_SENSOR_HPP
#define I_COLLISION_SENSOR_HPP

class ICollisionSensor {
public:
    virtual ~ICollisionSensor() {}
    virtual void update() = 0;
};

#endif
