#ifndef RIGID_BODY_HPP
#define RIGID_BODY_HPP

#include "component.hpp"
#include "../../common/util/log.hpp"

#include "collision_shape.hpp"

#include "../scene/game_object.hpp"
#include "../scene/game_scene.hpp"

class RigidBody : public Attribute {
    RTTR_ENABLE(Attribute)
public:
    virtual void onCreate();
    ~RigidBody();

    virtual void copy(Attribute* other);

    void _updateTransform();

    void _updateTransformFromBody();

    void _shapeChanged();

    btRigidBody* getBtBody() {
        return cobj.get();
    }

    virtual bool serialize(out_stream& out) {
        out.write(mass);
        return true;
    }
    virtual bool deserialize(in_stream& in, size_t sz) {
        mass = in.read<float>();
        _shapeChanged();
        return true;
    }
private:
    std::shared_ptr<btRigidBody> cobj;
    std::shared_ptr<btDefaultMotionState> motionState;
    float mass = 1.0f;
    std::shared_ptr<CollisionShape> shape = 0;
};

#endif
