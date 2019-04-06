#ifndef GHOST_OBJECT_HPP
#define GHOST_OBJECT_HPP

#include "component.hpp"
#include "../../common/util/log.hpp"

#include "collision_shape.hpp"

#include "../scene/game_object.hpp"
#include "../scene/game_scene.hpp"
#include "../scene/controllers/dynamics_ctrl.hpp"

class GhostObject : public ObjectComponent {
    RTTR_ENABLE(ObjectComponent)
public:

    gfxm::vec3 getCollisionAdjustedPosition() {
        gfxm::vec3 p = getOwner()->getTransform()->getWorldPosition();
        if(getOwner()->getScene()
            ->getController<DynamicsCtrl>()->getAdjustedPosition(
                cobj.get(), p
            )
        ) {
            return p;
        }
        return getOwner()->getTransform()->getWorldPosition();
    }

    void sweep(const gfxm::mat4& from, const gfxm::mat4& to) {
        //auto btWorld = getOwner()->getScene()->getController<DynamicsCtrl>()->getBtWorld();
        getOwner()->getScene()->getController<DynamicsCtrl>()->sweepTest(this, from, to);
    }

    virtual void onCreate() {
        shape = getOwner()->get<CmCollisionShape>();
        cobj.reset(new btGhostObject());
        cobj->setCustomDebugColor(
            btVector3(1, 0, 1)
        );
        cobj->setCollisionFlags(
            cobj->getCollisionFlags()
            | btCollisionObject::CF_HAS_CUSTOM_DEBUG_RENDERING_COLOR
        );
        _shapeChanged();
    }
    ~GhostObject() {
        getOwner()->getScene()
            ->getController<DynamicsCtrl>()->_removeGhost(this);
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
        getOwner()->getScene()
            ->getController<DynamicsCtrl>()->_removeGhost(this);
        cobj->setCollisionShape(shape->getBtShape());
        if(shape->getBtShape()) {
            getOwner()->getScene()
                ->getController<DynamicsCtrl>()->_addGhost(this);
        }
    }

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
    std::shared_ptr<CmCollisionShape> shape = 0;
};

#endif
