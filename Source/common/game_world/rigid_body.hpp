#ifndef KT_ACTOR_RIGID_BODY_HPP
#define KT_ACTOR_RIGID_BODY_HPP

#include "actor.hpp"
#include "game_world.hpp"


class ktARigidBody : public ktActor, public btMotionState {
    RTTR_ENABLE(ktActor)

    std::unique_ptr<btRigidBody> bt_rigid_body;
    std::unique_ptr<btCollisionShape> bt_shape;
public:
    ktARigidBody() {
        float mass = 1.0f;
        btVector3 local_inertia;
        bt_shape.reset(
            new btBoxShape(btVector3(0.5f, 0.5f, 0.5f))
        );
        bt_shape->calculateLocalInertia(
            mass, local_inertia
        );
        bt_rigid_body.reset(
            new btRigidBody(
                mass,
                this,
                bt_shape.get(),
                local_inertia
            )
        );
    }
    ~ktARigidBody() {

    }

    // ktActor
    void onSpawn(ktGameWorld* world) override {
        world->bt_world->addRigidBody(bt_rigid_body.get());
    }
    void onDespawn(ktGameWorld* world) override {
        world->bt_world->removeRigidBody(bt_rigid_body.get());
    }

    void onGui() override {
        ktActor::onGui();
    }

    // btMotionState
private:
    void getWorldTransform(btTransform& transform) const override {
        const gfxm::mat4& t = const_cast<ktARigidBody*>(this)->ktActor::getWorldTransform();
        transform.setFromOpenGLMatrix((float*)&t);
    }
    void setWorldTransform(const btTransform& transform) override {
        auto bt_vec3 = transform.getOrigin();
        setTranslation(gfxm::vec3(bt_vec3.getX(), bt_vec3.getY(), bt_vec3.getZ()));
        auto bt_quat = transform.getRotation();
        setRotation(gfxm::quat(bt_quat.getX(), bt_quat.getY(), bt_quat.getZ(), bt_quat.getW()));
    }
};
STATIC_RUN(ktARigidBody) {
    rttr::registration::class_<ktARigidBody>("RigidBody")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}


#endif
