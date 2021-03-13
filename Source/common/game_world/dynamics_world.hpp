#ifndef KT_DYNAMICS_WORLD_HPP
#define KT_DYNAMICS_WORLD_HPP

#include <memory>
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include "../util/bullet_debug_draw.hpp"

#include <game_world/transform.hpp>


struct phyCollisionObject : public ktDirtyArrayElement {
    hTransform                          transform;
    std::unique_ptr<btRigidBody>        body;
    std::unique_ptr<btCollisionShape>   shape;
    std::unique_ptr<btDefaultMotionState> motion_state;
};

struct phyRigidBody {
    hTransform                          transform;
    std::unique_ptr<btRigidBody>        body;
    std::unique_ptr<btCollisionShape>   shape;
    std::unique_ptr<btDefaultMotionState> motion_state;
};

class ktDynamicsWorld {
    std::unique_ptr<btDefaultCollisionConfiguration>        collision_conf;
    std::unique_ptr<btCollisionDispatcher>                  collision_dispatcher;
    std::unique_ptr<btDbvtBroadphase>                       collision_broadphase;
    std::unique_ptr<btSequentialImpulseConstraintSolver>    constraint_solver;
    std::unique_ptr<btDiscreteDynamicsWorld>                bt_world;

    BulletDebugDrawer2_OpenGL                               btDebugDrawer;

    ktDirtyArray<phyCollisionObject> collision_object_array;
    std::set<phyRigidBody*> rigid_bodies;

public:
    ktDynamicsWorld() {
        collision_conf.reset(new btDefaultCollisionConfiguration());
        collision_dispatcher.reset(new btCollisionDispatcher(collision_conf.get()));
        collision_broadphase.reset(new btDbvtBroadphase());
        constraint_solver.reset(new btSequentialImpulseConstraintSolver());
        bt_world.reset(new btDiscreteDynamicsWorld(
            collision_dispatcher.get(), 
            collision_broadphase.get(), 
            constraint_solver.get(),
            collision_conf.get()
        ));
        collision_broadphase->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());
        bt_world->setGravity(btVector3(0.0f, -10.0f, 0.0f));
        bt_world->setDebugDrawer(&btDebugDrawer);

        auto plane_object = new btCollisionObject();
        auto plane_shape = new btStaticPlaneShape(btVector3(0, 1, 0), 0.0f);
        plane_object->setCollisionShape(plane_shape);
        bt_world->addCollisionObject(plane_object);
    }

    btDiscreteDynamicsWorld* getBtWorld() { return bt_world.get(); }
    BulletDebugDrawer2_OpenGL* getDebugDraw() { return &btDebugDrawer; }

    void addCollisionObject(phyCollisionObject* o) {
        collision_object_array.add(o);
        o->transform->linkDirtyArray(&collision_object_array, o);

        bt_world->addRigidBody(o->body.get());
    }
    void removeCollisionObject(phyCollisionObject* o) {
        bt_world->removeRigidBody(o->body.get());

        o->transform->unlinkDirtyArray(&collision_object_array, o);
        collision_object_array.remove(o);
    }
    void addRigidBody(phyRigidBody* b) {
        rigid_bodies.insert(b);
        bt_world->addRigidBody(b->body.get());
    }
    void removeRigidBody(phyRigidBody* b) {
        bt_world->removeRigidBody(b->body.get());
        rigid_bodies.erase(b);
    }

    void update(float dt) {
        for(int i = 0; i < collision_object_array.dirtyCount(); ++i) {
            auto p = collision_object_array[i];
            const gfxm::mat4& m = p->transform->getWorldTransform();
            btTransform bt_transform;
            bt_transform.setFromOpenGLMatrix((float*)&m);
            p->body->setWorldTransform(bt_transform);
        }
        collision_object_array.clearDirtyCount();

        bt_world->stepSimulation(dt);

        for(auto b : rigid_bodies) {
            const btTransform& bt_transform = b->body->getWorldTransform();
            gfxm::mat4 m = gfxm::mat4(1.0f);
            bt_transform.getOpenGLMatrix((float*)&m);
            btVector3 bt_t = bt_transform.getOrigin();
            btQuaternion bt_r = bt_transform.getRotation();
            b->transform->setTranslation(gfxm::vec3(bt_t.getX(), bt_t.getY(), bt_t.getZ()));
            b->transform->setRotation(gfxm::quat(bt_r.getX(), bt_r.getY(), bt_r.getZ(), bt_r.getW()));
        }
    }
};


#endif
