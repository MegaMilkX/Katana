#ifndef ECS_DYNAMICS_SYS_HPP
#define ECS_DYNAMICS_SYS_HPP


#include "../system.hpp"
#include "../attribs/base_attribs.hpp"

#include "../../util/bullet_debug_draw.hpp"


class ecsArchCollider : public ecsTuple<ecsWorldTransform, ecsCollisionShape, ecsExclude<ecsMass>> {
public:
    void onAttribUpdate(ecsCollisionShape* shape) override {
        world->removeCollisionObject(collision_object.get());
        collision_object->setCollisionShape(shape->shape.get());
        world->addCollisionObject(collision_object.get());
        //LOG("Collider shape changed");
    }

    btCollisionWorld* world = 0;
    std::shared_ptr<btCollisionObject> collision_object;
};
class ecsArchRigidBody : public ecsTuple<ecsWorldTransform, ecsCollisionShape, ecsMass> {
public:
    void onAttribUpdate(ecsWorldTransform* t) override {
        //auto& transform = rigid_body->getWorldTransform();        
        btTransform transform;
        const gfxm::mat4& world_transform = t->transform;
        transform.setFromOpenGLMatrix((float*)&world_transform);
        
        //LOG(world_transform);
        rigid_body->setWorldTransform(transform);
        world->updateSingleAabb(rigid_body.get());
    }

    void onAttribUpdate(ecsMass* m) override {
        rigid_body->setMassProps(m->mass, btVector3(.0f, .0f, .0f));
    }

    void onAttribUpdate(ecsCollisionShape* shape) override {
        world->removeRigidBody(rigid_body.get());
        rigid_body->setCollisionShape(shape->shape.get());
        world->addRigidBody(rigid_body.get());
        //LOG("RigidBody shape changed");
    }

    btDiscreteDynamicsWorld* world = 0;
    std::shared_ptr<btRigidBody> rigid_body;
    btDefaultMotionState motion_state;
};


class ecsDynamicsSys : public ecsSystem<
    ecsArchCollider,
    ecsArchRigidBody
> {
    btDefaultCollisionConfiguration* collisionConf;
    btCollisionDispatcher* dispatcher;
    btDbvtBroadphase* broadphase;
    btSequentialImpulseConstraintSolver* constraintSolver;
    btDiscreteDynamicsWorld* world;

    BulletDebugDrawer2_OpenGL debugDrawer;
public:
    ecsDynamicsSys() {
        collisionConf = new btDefaultCollisionConfiguration();
        dispatcher = new btCollisionDispatcher(collisionConf);
        broadphase = new btDbvtBroadphase();

        constraintSolver = new btSequentialImpulseConstraintSolver();
        world = new btDiscreteDynamicsWorld(
            dispatcher, 
            broadphase, 
            constraintSolver,
            collisionConf
        );

        broadphase->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());

        world->setGravity(btVector3(0.0f, -10.0f, 0.0f));

        world->setDebugDrawer(&debugDrawer);
    }

    void setDebugDraw(DebugDraw* dd) {
        debugDrawer.setDD(dd);
    }

    void onFit(ecsArchCollider* collider) {
        collider->world = world;
        collider->collision_object.reset(new btCollisionObject());
        collider->collision_object->setCollisionShape(
            collider->get<ecsCollisionShape>()->shape.get()
        );

        btTransform btt;
        btt.setFromOpenGLMatrix((float*)&collider->get<ecsWorldTransform>()->transform);
        collider->collision_object->setWorldTransform(btt);

        world->addCollisionObject(collider->collision_object.get());
    }
    void onUnfit(ecsArchCollider* collider) {
        world->removeCollisionObject(
            collider->collision_object.get()
        );
    }
    void onFit(ecsArchRigidBody* rb) {
        rb->world = world;
        btVector3 local_inertia;
        rb->get<ecsCollisionShape>()->shape->calculateLocalInertia(
            rb->get<ecsMass>()->mass,
            local_inertia
        );
        btTransform btt, com;
        btt.setFromOpenGLMatrix((float*)&rb->get<ecsWorldTransform>()->transform);
        com.setIdentity();
        rb->motion_state = btDefaultMotionState(
            btt, com
        );
        
        rb->rigid_body.reset(new btRigidBody(
            rb->get<ecsMass>()->mass,
            &rb->motion_state,
            rb->get<ecsCollisionShape>()->shape.get(),
            local_inertia
        ));

        world->addRigidBody(
            rb->rigid_body.get()
        );
    }
    void onUnfit(ecsArchRigidBody* rb) {
        world->removeRigidBody(
            rb->rigid_body.get()
        );
    }

    void onUpdate() {
        for(auto& a : get_array<ecsArchCollider>()) {
            auto& matrix = a->get<ecsWorldTransform>()->transform;
            a->collision_object->getWorldTransform().setFromOpenGLMatrix((float*)&matrix);
            world->updateSingleAabb(a->collision_object.get());
        }

        world->stepSimulation(1.0f/60.0f);

        for(auto& a : get_array<ecsArchRigidBody>()) {
            auto& t = a->rigid_body->getWorldTransform();
            btVector3 btv3 = t.getOrigin();
            btQuaternion btq = t.getRotation();
            auto& transform = a->get<ecsWorldTransform>()->transform;
            t.getOpenGLMatrix((float*)&transform);
        }

        world->debugDrawWorld();
    }
};



#endif
