#ifndef ECS_DYNAMICS_SYS_HPP
#define ECS_DYNAMICS_SYS_HPP


#include "../system.hpp"
#include "../attribs/base_attribs.hpp"

#include "subscene_systems/sub_dynamics.hpp"

typedef ecsTuple<ecsTranslation, ecsRotation, ecsWorldTransform, ecsKinematicCapsule, ecsVelocity> ecsTupleKinematicCapsule;

class btKinematicMotionState : public btMotionState {
public:
    gfxm::vec3 translation = gfxm::vec3(0,0,0);
    gfxm::quat rotation = gfxm::quat(0,0,0,1);
    gfxm::mat4 transform = gfxm::mat4(1.0f);

    void getWorldTransform(btTransform& bt_transform) const override {
        const gfxm::mat4& t = this->transform;
        bt_transform.setFromOpenGLMatrix((float*)&t);
    }
    void setWorldTransform(const btTransform& bt_transform) override {
        auto bt_vec3 = bt_transform.getOrigin();
        translation = gfxm::vec3(bt_vec3.getX(), bt_vec3.getY(), bt_vec3.getZ());
        auto bt_quat = bt_transform.getRotation();
        rotation = gfxm::quat(bt_quat.getX(), bt_quat.getY(), bt_quat.getZ(), bt_quat.getW());
    }
};

class ContactCallback3 : public btCollisionWorld::ContactResultCallback {
public:
    bool hasContact = false;
    gfxm::vec3 penetration;
    gfxm::vec3 A;
    gfxm::vec3 B;

    btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1) override {
        if(hasContact) {
            return .0f;
        }
        if(cp.m_distance1 > .0f) {
            return .0f;
        }
        
        btVector3 btPosA = cp.m_positionWorldOnA;
        btVector3 btPosB = cp.m_positionWorldOnB;
        A = gfxm::vec3(btPosA.getX(), btPosA.getY(), btPosA.getZ());
        B = gfxm::vec3(btPosB.getX(), btPosB.getY(), btPosB.getZ());
        gfxm::vec3 P = B - A;

        penetration = P;

        hasContact = true;
        return .0f;
    }
};

class ConvexSweepCallback3 : public btCollisionWorld::ConvexResultCallback {
public:
    ConvexSweepCallback3(const gfxm::vec3& from, const gfxm::vec3& to, uint32_t mask)
    : from(from), to(to), mask(mask) {}

    virtual bool needsCollision(btBroadphaseProxy *proxy0) const {
        return proxy0->m_collisionFilterGroup & mask;
    }

    virtual btScalar addSingleResult(btCollisionWorld::LocalConvexResult &convexResult, bool normalInWorldSpace) {
        if(convexResult.m_hitCollisionObject->getInternalType() == btCollisionObject::CO_GHOST_OBJECT) {
            return 0.0f;
        }
        if(convexResult.m_hitCollisionObject->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE) {
            return 0.0f;
        }

        if(convexResult.m_hitFraction < closest_hit_fraction) {
            closest_hit_fraction = convexResult.m_hitFraction;
            closest_hit_center = gfxm::lerp(from, to, convexResult.m_hitFraction);
            closest_hit = closest_hit_center;
            has_hit = true;
        }

        gfxm::vec3 pos = gfxm::lerp(from, to, convexResult.m_hitFraction);

        return .0f;
    }

    bool hasHit() {
        return has_hit;
    }

    gfxm::vec3 closest_hit;
    gfxm::vec3 closest_hit_center;
    float closest_hit_fraction = 1.0f;
private:
    gfxm::vec3 from, to;
    bool has_hit = false;
    uint32_t mask;
};

class ecsDynamicsSys : public ecsSystem<
    ecsTupleCollisionSubScene,
    ecsArchCollider,
    ecsTupleCollisionPlane,
    ecsArchRigidBody,
    ecsTupleKinematicCharacter,
    ecsTupleCollisionCache,
    ecsTupleKinematicCapsule
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

    void onFit(ecsTupleKinematicCapsule* t) {
        auto capsule = t->get<ecsKinematicCapsule>();
        btVector3 local_inertia;
        float mass = 60.0f;
        capsule->motion_state = new btKinematicMotionState();
        ((btKinematicMotionState*)capsule->motion_state)->transform = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(.0f, 0.88f, .0f));
        capsule->capsule = new btCapsuleShape(0.330f, 0.810f);
        capsule->capsule->calculateLocalInertia(
            mass, local_inertia
        );
        capsule->rigid_body = new btRigidBody(mass, capsule->motion_state, capsule->capsule, local_inertia);
        capsule->rigid_body->setCollisionFlags(capsule->rigid_body->getCollisionFlags());
        capsule->rigid_body->setActivationState(DISABLE_DEACTIVATION);
        capsule->rigid_body->setAngularFactor(btVector3(0,0,0));
        capsule->rigid_body->setLinearFactor(btVector3(1,0,1));
        capsule->rigid_body->setDamping(.0f, .0f);

        world->addRigidBody(capsule->rigid_body, COLLISION_KINEMATIC_BIT, COLLISION_DEFAULT_BIT | COLLISION_DYNAMIC_BIT | COLLISION_KINEMATIC_BIT);
    }
    void onUnfit(ecsTupleKinematicCapsule* t) {
        auto capsule = t->get<ecsKinematicCapsule>();
        world->removeRigidBody(capsule->rigid_body);
        delete capsule->rigid_body;
        delete capsule->capsule;
        delete capsule->motion_state;
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
        uint64_t group = COLLISION_DEFAULT_BIT;
        uint64_t mask = COLLISION_DYNAMIC_BIT | COLLISION_KINEMATIC_BIT;
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
        uint64_t group = COLLISION_DEFAULT_BIT;
        uint64_t mask = COLLISION_DYNAMIC_BIT | COLLISION_KINEMATIC_BIT;
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
        uint64_t group = COLLISION_DYNAMIC_BIT;
        uint64_t mask = COLLISION_DEFAULT_BIT | COLLISION_DYNAMIC_BIT | COLLISION_KINEMATIC_BIT;
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

    void onUpdate(float dt) {
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

        for(auto& a : get_array<ecsTupleKinematicCapsule>()) {
            auto transform = a->get<ecsWorldTransform>();
            auto capsule = a->get<ecsKinematicCapsule>();
            auto motion_state = ((btKinematicMotionState*)capsule->motion_state);
            auto velocity = a->get<ecsVelocity>();
            capsule->rigid_body->setLinearVelocity(*(btVector3*)&(velocity->velo));
            //capsule->rigid_body->translate(*(btVector3*)&(velocity->velo));                    
        }

        
        world->stepSimulation(dt, 1, dt);

        for(auto& a : get_array<ecsTupleKinematicCapsule>()) {
            auto position = a->get<ecsTranslation>();
            auto rotation = a->get<ecsRotation>();
            auto capsule = a->get<ecsKinematicCapsule>();
            auto motion_state = ((btKinematicMotionState*)capsule->motion_state);
            gfxm::mat4 m;
            capsule->rigid_body->getWorldTransform().getOpenGLMatrix((float*)&m);
            position->setPosition(m * gfxm::vec4(0,-0.88f,0,1));       
        }

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
                    float penetration = -btpt.getDistance();

                    if(cacheEntityA) {
                        ecsCollisionCache::CollisionPoint pt;
                        pt.pointOnA = gfxm::vec3(ptA.getX(), ptA.getY(), ptA.getZ());
                        pt.pointOnB = gfxm::vec3(ptB.getX(), ptB.getY(), ptB.getZ());
                        pt.normalOnB = gfxm::vec3(normalOnB.getX(), normalOnB.getY(), normalOnB.getZ());
                        pt.penetration = penetration;
                        cacheEntityA->points.push_back(pt);
                    }
                    if(cacheEntityB) {
                        ecsCollisionCache::CollisionPoint pt;
                        pt.pointOnA = gfxm::vec3(ptB.getX(), ptB.getY(), ptB.getZ());
                        pt.pointOnB = gfxm::vec3(ptA.getX(), ptA.getY(), ptA.getZ());
                        pt.normalOnB = -gfxm::vec3(normalOnB.getX(), normalOnB.getY(), normalOnB.getZ());
                        pt.penetration = penetration;
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
