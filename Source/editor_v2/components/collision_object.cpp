#include "collision_object.hpp"

STATIC_RUN(CollisionObject) {
    rttr::registration::class_<CollisionObject>("CollisionObject")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#include "../scene/controllers/dynamics_ctrl.hpp"

void CollisionObject::onCreate() {
    shape = getOwner()->get<CollisionShape>();
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
CollisionObject::~CollisionObject() {
    getOwner()->getScene()->getController<DynamicsCtrl>()->_removeCollider(this);
}

void CollisionObject::_updateTransform() {
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

void CollisionObject::_shapeChanged() {
    getOwner()->getScene()->getController<DynamicsCtrl>()->_removeCollider(this);
    cobj->setCollisionShape(shape->getBtShape());
    if(shape->getBtShape()) {
        if(shape->getShapeType() == rttr::type::get<MeshShape>()) {
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