#ifndef GHOST_OBJECT_HPP
#define GHOST_OBJECT_HPP

#include "component.hpp"
#include "../../common/util/log.hpp"

#include "collision_shape.hpp"

#include "../scene/game_object.hpp"
#include "../scene/game_scene.hpp"

class GhostObject : public Attribute {
    RTTR_ENABLE(Attribute)
public:

    gfxm::vec3 getCollisionAdjustedPosition();

    void sweep(const gfxm::mat4& from, const gfxm::mat4& to);

    virtual void onCreate();
    ~GhostObject();

    void _updateTransform() {
        if(!shape) return;
        gfxm::vec3 t = getOwner()->getTransform()->getWorldPosition();
        gfxm::quat r = getOwner()->getTransform()->getWorldRotation();
        gfxm::vec3 s = getOwner()->getTransform()->getWorldScale();
        t = t + gfxm::vec3(gfxm::to_mat4(r) * gfxm::vec4(shape->getOffset(), 1.0f));
        btTransform btt;
        btt.setRotation(btQuaternion(r.x, r.y, r.z, r.w));
        btt.setOrigin(btVector3(t.x, t.y, t.z));
        shape->getBtShape()->setLocalScaling(btVector3(s.x, s.y, s.z));
        cobj->setWorldTransform(btt);
    }

    void _shapeChanged();

    btGhostObject* getBtObject() {
        return cobj.get();
    }

    virtual bool serialize(out_stream& out) {
        return true;
    }
    virtual bool deserialize(in_stream& in, size_t sz) {
        return true;
    }
private:
    std::shared_ptr<btGhostObject> cobj;
    std::shared_ptr<CollisionShape> shape = 0;
};

#endif
