#ifndef COLLISION_OBJECT_HPP
#define COLLISION_OBJECT_HPP

#include "component.hpp"
#include "../../common/util/log.hpp"

#include "collision_shape.hpp"

#include "../scene/game_object.hpp"
#include "../scene/game_scene.hpp"
#include "../scene/controllers/dynamics_ctrl.hpp"

class CmCollisionObject : public ObjectComponent {
    RTTR_ENABLE(ObjectComponent)
public:
    virtual void onCreate() {
        shape = getOwner()->get<CmCollisionShape>();
        cobj.reset(new btCollisionObject());
        cobj->setCustomDebugColor(
            btVector3(0, 1, 0)
        );
        cobj->setCollisionFlags(
            cobj->getCollisionFlags()
            | btCollisionObject::CF_HAS_CUSTOM_DEBUG_RENDERING_COLOR
            | btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT
        );
        _shapeChanged();
    }
    ~CmCollisionObject() {
        getOwner()->getScene()->getController<DynamicsCtrl>()->_removeCollider(this);
    }

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

    void _shapeChanged() {
        getOwner()->getScene()->getController<DynamicsCtrl>()->_removeCollider(this);
        cobj->setCollisionShape(shape->getBtShape());
        if(shape->getBtShape()) {
            if(shape->getShapeType() == rttr::type::get<MeshShape_>()) {
                cobj->setCollisionFlags(
                    cobj->getCollisionFlags()
                    | btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT
                );
            } else {
                cobj->setCollisionFlags(
                    cobj->getCollisionFlags()
                    & ~btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT
                );
            }
            getOwner()->getScene()->getController<DynamicsCtrl>()->_addCollider(this);
        }
    }

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
    std::shared_ptr<CmCollisionShape> shape = 0;
};

#endif
