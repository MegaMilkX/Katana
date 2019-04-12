#ifndef COLLISION_OBJECT_HPP
#define COLLISION_OBJECT_HPP

#include "component.hpp"
#include "../../common/util/log.hpp"

#include "collision_shape.hpp"

#include "../scene/game_object.hpp"
#include "../scene/game_scene.hpp"

class CollisionObject : public Attribute {
    RTTR_ENABLE(Attribute)
public:
    virtual void onCreate();
    ~CollisionObject();

    void _updateTransform();

    void _shapeChanged();

    btCollisionObject* getBtObject() {
        return cobj.get();
    }

    virtual bool serialize(out_stream& out) {
        return true;
    }
    virtual bool deserialize(in_stream& in, size_t sz) {
        return true;
    }
private:
    std::shared_ptr<btCollisionObject> cobj;
    std::shared_ptr<CollisionShape> shape = 0;
};

#endif
