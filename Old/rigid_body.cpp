#include "rigid_body.hpp"

STATIC_RUN(RigidBody) {
    rttr::registration::class_<RigidBody>("RigidBody")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#include "../scene/controllers/dynamics_ctrl.hpp"

void RigidBody::onCreate() {
    shape = getOwner()->get<CollisionShape>();
    _shapeChanged();
}
RigidBody::~RigidBody() {
    getOwner()->getScene()->getController<DynamicsCtrl>()->_removeRigidBody(this);
}

void RigidBody::copy(Attribute* other) {
    if(other->get_type() != get_type()) {
        LOG("Can't copy from " << other->get_type().get_name().to_string() << " to " <<
            get_type().get_name().to_string());
        return;
    }
    RigidBody* s = (RigidBody*)other;

    mass = s->mass;
    _shapeChanged();
}

void RigidBody::_updateTransform() {
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

void RigidBody::_updateTransformFromBody() {
    if(cobj) {
        auto t = cobj->getWorldTransform();
        gfxm::mat4 m;
        t.getOpenGLMatrix((float*)&m);
        getOwner()->getTransform()->setTransform(m);
    }
}

void RigidBody::_shapeChanged() {
    getOwner()->getScene()->getController<DynamicsCtrl>()->_removeRigidBody(this);
    if(shape->getBtShape()) {
        btTransform transform;
        transform.setFromOpenGLMatrix((float*)&getOwner()->getTransform()->getWorldTransform());
        motionState.reset(new btDefaultMotionState(transform));
        btVector3 localInertia(0, 0, 0);
        shape->getBtShape()->calculateLocalInertia(mass, localInertia);
        btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState.get(), shape->getBtShape(), localInertia);
        cobj.reset(new btRigidBody(rbInfo));

        getOwner()->getScene()->getController<DynamicsCtrl>()->_addRigidBody(this);
    }
}