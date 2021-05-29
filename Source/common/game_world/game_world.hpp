#ifndef ECS_GAME_SCENE_HPP
#define ECS_GAME_SCENE_HPP

#include <memory>

#include "resource/resource.h"
#include "ecs/world.hpp"

#include "ecs/attribs/model.hpp"

#include "../util/bullet_debug_draw.hpp"

#include "actor.hpp"

class ktGameWorld : public Resource {
    RTTR_ENABLE(Resource);

    std::unique_ptr<ecsWorld> ecs_world;

    std::set<ktActor*> actors;

public:
    std::unique_ptr<btDefaultCollisionConfiguration> collisionConf;
    std::unique_ptr<btCollisionDispatcher> dispatcher;
    std::unique_ptr<btDbvtBroadphase> broadphase;
    std::unique_ptr<btSequentialImpulseConstraintSolver> constraintSolver;
    std::unique_ptr<btDiscreteDynamicsWorld> bt_world;

    BulletDebugDrawer2_OpenGL debugDrawer;

    ktGameWorld() {
        ecs_world.reset(new ecsWorld);

        collisionConf.reset(new btDefaultCollisionConfiguration());
        dispatcher.reset(new btCollisionDispatcher(collisionConf.get()));
        broadphase.reset(new btDbvtBroadphase());
        constraintSolver.reset(new btSequentialImpulseConstraintSolver());
        bt_world.reset(new btDiscreteDynamicsWorld(
            dispatcher.get(), 
            broadphase.get(), 
            constraintSolver.get(),
            collisionConf.get()
        ));
        broadphase->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());
        bt_world->setGravity(btVector3(0.0f, -10.0f, 0.0f));
        bt_world->setDebugDrawer(&debugDrawer);
    }
    ~ktGameWorld() {
    }

    void addActor(ktActor* actor) {
        actors.insert(actor);
        actor->onSpawn(this);
    }
    void removeActor(ktActor* actor) {
        actor->onDespawn(this);
        actors.erase(actor);
    }
    size_t actorCount() const {
        return actors.size();
    }
    ktActor* getActor(size_t idx) {
        auto it = actors.begin();
        std::advance(it, idx);
        return (*it);
    }

    void update(float dt) {
        for(auto a : actors) {
            a->onUpdate(dt);
        }
        
        bt_world->stepSimulation(dt);

        for(auto a : actors) {
            a->onPostCollisionUpdate(dt);
        }

        bt_world->debugDrawWorld();
    }

public:
    // Resource
    void    serialize(out_stream& out) override {
        // TODO
    }
    bool    deserialize(in_stream& in, size_t sz) override {
        // TODO
        return true;
    }
    const char* getWriteExtension() const override { return "world"; }
};
STATIC_RUN(ktGameWorld) {
    rttr::registration::class_<ecsWorld>("GameScene")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif

