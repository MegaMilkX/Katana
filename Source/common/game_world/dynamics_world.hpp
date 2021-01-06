#ifndef KT_DYNAMICS_WORLD_HPP
#define KT_DYNAMICS_WORLD_HPP

#include <memory>
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include "../util/bullet_debug_draw.hpp"

class ktDynamicsWorld {
    std::unique_ptr<btDefaultCollisionConfiguration>        collision_conf;
    std::unique_ptr<btCollisionDispatcher>                  collision_dispatcher;
    std::unique_ptr<btDbvtBroadphase>                       collision_broadphase;
    std::unique_ptr<btSequentialImpulseConstraintSolver>    constraint_solver;
    std::unique_ptr<btDiscreteDynamicsWorld>                bt_world;

    BulletDebugDrawer2_OpenGL                               btDebugDrawer;

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
};


#endif
