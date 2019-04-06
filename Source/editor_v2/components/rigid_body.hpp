#ifndef RIGID_BODY_HPP
#define RIGID_BODY_HPP

#include "component.hpp"
#include "../../common/util/log.hpp"

#include "collision_shape.hpp"

#include "../scene/game_object.hpp"
#include "../scene/game_scene.hpp"
#include "../scene/controllers/dynamics_ctrl.hpp"

class CmRigidBody : public ObjectComponent {
    RTTR_ENABLE(ObjectComponent)
public:
    virtual void onCreate() {
        shape = getOwner()->get<CmCollisionShape>();
        _shapeChanged();
    }
    ~CmRigidBody() {
        getOwner()->getScene()->getController<DynamicsCtrl>()->_removeRigidBody(this);
    }

    virtual void copy(ObjectComponent* other) {
        if(other->get_type() != get_type()) {
            LOG("Can't copy from " << other->get_type().get_name().to_string() << " to " <<
                get_type().get_name().to_string());
            return;
        }
        CmRigidBody* s = (CmRigidBody*)other;

        mass = s->mass;
        _shapeChanged();
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

    void _updateTransformFromBody() {
        if(cobj) {
            auto t = cobj->getWorldTransform();
            gfxm::mat4 m;
            t.getOpenGLMatrix((float*)&m);
            getOwner()->getTransform()->setTransform(m);
        }
    }

    void _shapeChanged() {
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
    std::shared_ptr<CmCollisionShape> shape = 0;
};

#endif
