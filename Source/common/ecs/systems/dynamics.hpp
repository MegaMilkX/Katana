#ifndef ECS_DYNAMICS_SYS_HPP
#define ECS_DYNAMICS_SYS_HPP


#include "../system.hpp"
#include "../attribs/base_attribs.hpp"

#include "subscene_systems/sub_dynamics.hpp"


class ecsDynamicsSys : public ecsSystem<
    ecsTupleCollisionSubScene,
    ecsArchCollider,
    ecsTupleCollisionPlane,
    ecsArchRigidBody,
    ecsTupleKinematicCharacter,
    ecsTupleCollisionCache
> {
    btDefaultCollisionConfiguration* collisionConf;
    btCollisionDispatcher* dispatcher;
    btDbvtBroadphase* broadphase;
    btSequentialImpulseConstraintSolver* constraintSolver;
    btDiscreteDynamicsWorld* world;

    using collision_pair_t = std::pair<const btCollisionObject*, const btCollisionObject*>;
    using collision_pair_set_t = std::set<collision_pair_t>;
    collision_pair_set_t pair_cache;

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

        world->setGravity(btVector3(0.0f, -9.8f, 0.0f));

        world->setDebugDrawer(&debugDrawer);
    }

    btDiscreteDynamicsWorld* getBtWorld() {
        return world;
    }

    void setDebugDraw(DebugDraw* dd) {
        debugDrawer.setDD(dd);
    }

    void onFit(ecsTupleCollisionSubScene* subscene) {
        subscene->reinit(world);
    }
    void onUnfit(ecsTupleCollisionSubScene* subscene) {}

    void onFit(ecsArchCollider* collider) {
        collider->world = world;
        collider->collision_object.reset(new btCollisionObject());
        collider->collision_object->setUserIndex((int)collider->getEntityUid()); // TODO: Bad, entity_id is 64 bit FIX THIS
        collider->collision_object->setUserIndex2(getWorld()->getWorldIndex());
        collider->collision_object->setCollisionShape(
            collider->get<ecsCollisionShape>()->shape.get()
        );
        if(collider->get_optional<ecsCollisionCache>()) {
            collider->collision_object->setUserPointer(collider->get_optional<ecsCollisionCache>());
        }

        btTransform btt;
        btt.setFromOpenGLMatrix((float*)&collider->get<ecsWorldTransform>()->getTransform());
        collider->collision_object->setWorldTransform(btt);

        ecsCollisionFilter* filter = collider->get_optional<ecsCollisionFilter>();
        uint64_t group = 1;
        uint64_t mask = 1;
        if(filter) {
            group = filter->group;
            mask = filter->mask;
        }

        world->addCollisionObject(collider->collision_object.get(), (int)group, (int)mask);
    }
    void onUnfit(ecsArchCollider* collider) {
        world->removeCollisionObject(
            collider->collision_object.get()
        );
    }
    void onFit(ecsTupleCollisionPlane* p) {
        p->world = world;
        p->collision_object.reset(new btCollisionObject());
        p->collision_object->setUserIndex((int)p->getEntityUid()); // TODO: Bad, entity_id is 64 bit FIX THIS
        p->collision_object->setUserIndex2(getWorld()->getWorldIndex());
        p->collision_object->setCollisionShape(
            p->get<ecsCollisionPlane>()->shape.get()
        );
        if(p->get_optional<ecsCollisionCache>()) {
            p->collision_object->setUserPointer(p->get_optional<ecsCollisionCache>());
        }

        ecsCollisionFilter* filter = p->get_optional<ecsCollisionFilter>();
        uint64_t group = 1;
        uint64_t mask = 1;
        if(filter) {
            group = filter->group;
            mask = filter->mask;
        }

        world->addCollisionObject(p->collision_object.get(), (int)group, (int)mask);
    }
    void onUnfit(ecsTupleCollisionPlane* p) {
        world->removeCollisionObject(
            p->collision_object.get()
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
        btt.setFromOpenGLMatrix((float*)&rb->get<ecsWorldTransform>()->getTransform());
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
        rb->rigid_body->setUserIndex((int)rb->getEntityUid()); // TODO: Bad, entity_id is 64 bit FIX THIS
        if(rb->get_optional<ecsCollisionCache>()) {
            rb->rigid_body->setUserPointer(rb->get_optional<ecsCollisionCache>());
        }

        ecsCollisionFilter* filter = rb->get_optional<ecsCollisionFilter>();
        uint64_t group = 1;
        uint64_t mask = 1;
        if(filter) {
            group = filter->group;
            mask = filter->mask;
        }

        world->addRigidBody(
            rb->rigid_body.get(), (int)group, (int)mask
        );
    }
    void onUnfit(ecsArchRigidBody* rb) {
        world->removeRigidBody(
            rb->rigid_body.get()
        );
    }

    void onUpdate() {
        for(int i = get_dirty_index<ecsArchCollider>(); i < count<ecsArchCollider>(); ++i) {
            auto tuple = get<ecsArchCollider>(i);
            if(tuple->is_dirty<ecsCollisionShape>()) {
                LOG_WARN("Collision shape updated: " << tuple->getEntityUid());
            }
            if(tuple->is_dirty<ecsWorldTransform>()) { // Update collider pos
                auto& matrix = tuple->get<ecsWorldTransform>()->getTransform();
                btTransform btt;
                btt.setFromOpenGLMatrix((float*)&matrix);
                tuple->collision_object->setWorldTransform(btt);
                world->updateSingleAabb(tuple->collision_object.get());
            }
            tuple->clear_dirty_signature();
            
        }
        clear_dirty<ecsArchCollider>();

        for(int i = get_dirty_index<ecsArchRigidBody>(); i < count<ecsArchRigidBody>(); ++i) {
            auto a = get<ecsArchRigidBody>(i);
            if(a->is_dirty<ecsWorldTransform>()) {
                // Update rigid body transform
            }
            a->clear_dirty_signature();
        }
        clear_dirty<ecsArchRigidBody>();

        for(auto& a : get_array<ecsTupleCollisionSubScene>()) {
            // TODO: Optimize?
            a->sub_dynamics->updateRootTransform(a->get<ecsWorldTransform>()->getTransform());
            a->sub_dynamics->updateColliders();
            // TODO
        }

        // Clear collision caches
        for(auto& a : get_array<ecsTupleCollisionCache>()) {
            a->get<ecsCollisionCache>()->entities.clear();
        }

        world->stepSimulation(1.0f/60.0f);

        for(auto& a : get_array<ecsArchRigidBody>()) {
            auto& t = a->rigid_body->getWorldTransform();
            btVector3 btv3 = t.getOrigin();
            btQuaternion btq = t.getRotation();
            auto& transform = a->get<ecsWorldTransform>()->getTransform();
            t.getOpenGLMatrix((float*)&transform);
        }

        collision_pair_set_t pairs;
        int numManifolds = world->getDispatcher()->getNumManifolds();
        for(int i = 0; i < numManifolds; ++i) {
            btPersistentManifold* contactManifold = world->getDispatcher()->getManifoldByIndexInternal(i);
            const btCollisionObject* A = contactManifold->getBody0();
            const btCollisionObject* B = contactManifold->getBody1();
            ecsCollisionCache* cacheA = 0;
            ecsCollisionCache* cacheB = 0;

            collision_pair_t p(A, B);
            pairs.insert(p);
            if(pair_cache.count(p) == 0) {
                // TODO: New collision, report
                //LOG_WARN("Collided");
            }

            ecsCollisionCache::Other* cacheEntityA = 0;
            ecsCollisionCache::Other* cacheEntityB = 0;
            if(A->getUserPointer()) {
                cacheA = (ecsCollisionCache*)A->getUserPointer();
                cacheA->entities.push_back(ecsCollisionCache::Other());
                cacheEntityA = &cacheA->entities.back();
                cacheEntityA->entity = ecsEntityHandle(derefWorldIndex(B->getUserIndex2()), (entity_id)B->getUserIndex());
            }
            if(B->getUserPointer()) {
                cacheB = (ecsCollisionCache*)B->getUserPointer();
                cacheB->entities.push_back(ecsCollisionCache::Other());
                cacheEntityB = &cacheB->entities.back();
                cacheEntityB->entity = ecsEntityHandle(derefWorldIndex(A->getUserIndex2()), (entity_id)A->getUserIndex());
            }

            int numContacts = contactManifold->getNumContacts();
            for(int j = 0; j < numContacts; ++j) {
                btManifoldPoint& btpt = contactManifold->getContactPoint(j);
                if(btpt.getDistance() < .0f) {
                    const btVector3& ptA = btpt.getPositionWorldOnA();
                    const btVector3& ptB = btpt.getPositionWorldOnB();
                    const btVector3& normalOnB = btpt.m_normalWorldOnB;

                    if(cacheEntityA) {
                        ecsCollisionCache::CollisionPoint pt;
                        pt.pointOnA = gfxm::vec3(ptA.getX(), ptA.getY(), ptA.getZ());
                        pt.pointOnB = gfxm::vec3(ptB.getX(), ptB.getY(), ptB.getZ());
                        pt.normalOnB = gfxm::vec3(normalOnB.getX(), normalOnB.getY(), normalOnB.getZ());
                        cacheEntityA->points.push_back(pt);
                    }
                    if(cacheEntityB) {
                        ecsCollisionCache::CollisionPoint pt;
                        pt.pointOnA = gfxm::vec3(ptB.getX(), ptB.getY(), ptB.getZ());
                        pt.pointOnB = gfxm::vec3(ptA.getX(), ptA.getY(), ptA.getZ());
                        pt.normalOnB = -gfxm::vec3(normalOnB.getX(), normalOnB.getY(), normalOnB.getZ());
                        cacheEntityB->points.push_back(pt);
                    }

                    // TODO: ?
                }
            }
        }
        collision_pair_set_t missing_pairs;
        std::set_difference(pair_cache.begin(), pair_cache.end(), pairs.begin(), pairs.end(), std::inserter(missing_pairs, missing_pairs.begin()));
        pair_cache = pairs;
        for(auto& p : missing_pairs) {
            // TODO: Collision ended, report
            //LOG_WARN("CollisionEnded");
        }

        world->debugDrawWorld();
    }
};



#endif
