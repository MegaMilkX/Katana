#include "ghost_object.hpp"

STATIC_RUN(GhostObject) {
    rttr::registration::class_<GhostObject>("GhostObject")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#include "../scene/controllers/dynamics_ctrl.hpp"

void GhostObject::onCreate() {
    shape = getOwner()->get<CollisionShape>();
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
GhostObject::~GhostObject() {
    getOwner()->getScene()
        ->getController<DynamicsCtrl>()->_removeGhost(this);
}

gfxm::vec3 GhostObject::getCollisionAdjustedPosition() {
    gfxm::vec3 p = getOwner()->getTransform()->getWorldPosition();
    if(getOwner()->getScene()
        ->getController<DynamicsCtrl>()->getAdjustedPosition(
            cobj.get(), p, 1
        )
    ) {
        return p;
    }
    return getOwner()->getTransform()->getWorldPosition();
}

void GhostObject::sweep(const gfxm::mat4& from, const gfxm::mat4& to) {
    //auto btWorld = getOwner()->getScene()->getController<DynamicsCtrl>()->getBtWorld();
    getOwner()->getScene()->getController<DynamicsCtrl>()->sweepTest(this, from, to);
}

void GhostObject::_shapeChanged() {
    getOwner()->getScene()
        ->getController<DynamicsCtrl>()->_removeGhost(this);
    cobj->setCollisionShape(shape->getBtShape());
    if(shape->getBtShape()) {
        getOwner()->getScene()
            ->getController<DynamicsCtrl>()->_addGhost(this);
    }
}