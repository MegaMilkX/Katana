#include "dynamics_ctrl.hpp"

#include "../node.hpp"

#include "../../attributes/rigid_body.hpp"

class ConvexSweepCallback_ : public btCollisionWorld::ConvexResultCallback {
public:
    ConvexSweepCallback_(DynamicsCtrl* wrld, float r, const gfxm::vec3& v_from, const gfxm::vec3& v_to, uint32_t mask)
    : world(wrld), radius(r), from(v_from), to(v_to), mask(mask) {}

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
            gfxm::vec3 radius_vec = gfxm::normalize(to - from) * radius;
            closest_hit_center = gfxm::lerp(from, to, convexResult.m_hitFraction);
            closest_hit = closest_hit_center;
            has_hit = true;
        }

        gfxm::vec3 pos = gfxm::lerp(from, to, convexResult.m_hitFraction);
        
        world->getBtWorld()->getDebugDrawer()->drawSphere(
            radius,
            *(btTransform*)&gfxm::translate(gfxm::mat4(1.0f), pos),
            btVector3(0.6f, .3f, .0f)
        );

        return .0f;
    }

    bool hasHit() {
        return has_hit;
    }

    gfxm::vec3 closest_hit;
    gfxm::vec3 closest_hit_center;
    float closest_hit_fraction = 1.0f;
private:
    DynamicsCtrl* world = 0;
    gfxm::vec3 from;
    gfxm::vec3 to;
    float radius;
    bool has_hit = false;
    uint32_t mask;
};

class ContactAdjustCallback_ : public btCollisionWorld::ContactResultCallback {
public:
    ContactAdjustCallback_(DynamicsCtrl* wrld, gfxm::vec3& pos, uint32_t mask)
    : world(wrld), pos(pos), mask(mask) {

    }

    virtual bool 	needsCollision (btBroadphaseProxy *proxy0) const {
        return proxy0->m_collisionFilterGroup & mask;
    }

    virtual btScalar addSingleResult(
        btManifoldPoint& cp, 
        const btCollisionObjectWrapper* colObj0Wrap, 
        int partId0, 
        int index0, 
        const btCollisionObjectWrapper* colObj1Wrap, 
        int partId1, 
        int index1
    ) {
        if(colObj1Wrap->getCollisionObject()->getInternalType() == btCollisionObject::CO_GHOST_OBJECT) {
            return 0.0f;
        }
        if(colObj1Wrap->getCollisionObject()->getCollisionFlags() & btCollisionObject::CF_NO_CONTACT_RESPONSE) {
            return 0.0f;
        }
        if(cp.getDistance() >= 0.0f) {
            return 0.0f;
        }

        world->getBtWorld()->getDebugDrawer()->drawSphere(
            0.1f,
            *(btTransform*)&gfxm::translate(gfxm::mat4(1.0f), *(gfxm::vec3*)&cp.getPositionWorldOnB()),
            btVector3(0.3f, .0f, .0f)
        );
        world->getBtWorld()->getDebugDrawer()->drawSphere(
            0.1f,
            *(btTransform*)&gfxm::translate(gfxm::mat4(1.0f), *(gfxm::vec3*)&cp.getPositionWorldOnA()),
            btVector3(0.0f, .3f, .0f)
        );
        world->getBtWorld()->getDebugDrawer()->drawLine(
            cp.m_positionWorldOnB,
            cp.m_positionWorldOnA,
            btVector3(1.0f, .0f, .0f)
        );

        btVector3 nrm = cp.m_positionWorldOnB - cp.m_positionWorldOnA;
        gfxm::vec3 N = gfxm::normalize(gfxm::vec3(nrm.getX(), nrm.getY(), nrm.getZ()));
        float dist = cp.getDistance();
        
        pos += N * (fabs(dist * dist));//std::min(gfxm::sqrt(-dist), 1.0f);
        
        has_hit = true;

        return 1.0f;
    }

    bool hasHit() {
        return has_hit;
    }

    gfxm::vec3 hit_normal;
    gfxm::vec3 pos;
private:
    uint32_t mask;
    bool has_hit = false;
    DynamicsCtrl* world = 0;
    
};

DynamicsCtrl::DynamicsCtrl() {
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

    col_group_names[0] = "Static";
    col_group_names[1] = "Dynamic";
    col_group_names[2] = "Trigger";
    col_group_names[3] = "Actor";
    col_group_names[4] = "Hitbox";
    col_group_names[5] = "Hurtbox";
}
DynamicsCtrl::~DynamicsCtrl() {
    delete world;
    delete constraintSolver;
    delete broadphase;
    delete dispatcher;
    delete collisionConf;
}

bool DynamicsCtrl::getAdjustedPosition(btCollisionObject* o, gfxm::vec3& pos, uint32_t mask) {
    ContactAdjustCallback_ cb(this, pos, mask);
    world->contactTest(o, cb);
    if(cb.hasHit()) {
        pos = cb.pos;
        return true;
    }
    return false;
}
bool DynamicsCtrl::getAdjustedPosition(Collider* c, gfxm::vec3& pos, uint32_t mask) {
    auto it = colliders_.find(c);
    if(it == colliders_.end()) {
        return false;
    }
    auto btco = it->second.bt_object.get();
    return getAdjustedPosition(btco, pos, mask);
}

bool DynamicsCtrl::sweepSphere(float radius, const gfxm::vec3& from, const gfxm::vec3& to, gfxm::vec3& hit, uint32_t mask) {
    btSphereShape shape(radius);
    btTransform tfrom;
    btTransform tto;
    tfrom.setIdentity();
    tto.setIdentity();
    tfrom.setOrigin(btVector3(from.x, from.y, from.z));
    tto.setOrigin(btVector3(to.x, to.y, to.z));
    ConvexSweepCallback_ cb(this, radius, from, to, mask);
    
    world->convexSweepTest(
        &shape,
        tfrom,
        tto,
        cb
    );
    
    world->getDebugDrawer()->drawSphere(
        radius,
        tfrom,
        btVector3(1.0f, 1.0f, .0f)
    );

    if(cb.hasHit()) {
        hit = cb.closest_hit;
        btTransform to;
        to.setIdentity();
        to.setOrigin(btVector3(hit.x, hit.y, hit.z));
        world->getDebugDrawer()->drawSphere(
            radius,
            to,
            btVector3(1.0f, 0.5f, .0f)
        );
    } else {
        world->getDebugDrawer()->drawSphere(
            radius,
            tto,
            btVector3(1.0f, 0.5f, .0f)
        );
    }
    return cb.hasHit();
}

btDynamicsWorld* DynamicsCtrl::getBtWorld() {
    return world;
}

void DynamicsCtrl::update(float dt) {
    for(auto& kv : colliders_) {
        auto& cinf = kv.second;
        auto& c = kv.first;
        auto t = c->getOwner()->getTransform();
        if(t->getSyncId() != cinf.transform_sync_id) {
            btTransform btt;
            auto m = t->getWorldTransform();
            auto p = t->getWorldPosition();
            auto r = t->getWorldRotation();
            auto s = t->getWorldScale();
            auto o = c->getOffset();
            o = m * gfxm::vec4(o, .0f);
            p = p + o;
            btt.setOrigin(btVector3(p.x, p.y, p.z));
            btt.setRotation(btQuaternion(r.x, r.y, r.z, r.w));
            cinf.bt_object->getCollisionShape()->setLocalScaling(btVector3(s.x, s.y, s.z));
            
            cinf.bt_object->setWorldTransform(btt);
            cinf.transform_sync_id = t->getSyncId();   
        }
    }

    // =================================
    world->stepSimulation(dt);
    // =================================

    std::set<std::pair<ktNode*, ktNode*>> pairs;
    int manifold_count = dispatcher->getNumManifolds();
    for(int i = 0; i < manifold_count; ++i) {
        auto m = dispatcher->getManifoldByIndexInternal(i);
        if(m->getNumContacts() > 0) {
            auto p = std::make_pair(
                (ktNode*)m->getBody0()->getUserPointer(), 
                (ktNode*)m->getBody1()->getUserPointer()
            );
            pairs.insert(p);
            if(pair_cache.count(p) == 0) {
                auto it = col_listeners.find(p.first);
                if(it != col_listeners.end()) {
                    it->second->onEnter(p.second);
                }
                it = col_listeners.find(p.second);
                if(it != col_listeners.end()) {
                    it->second->onEnter(p.first);
                }
            }
            
        }
    }
    
    std::set<std::pair<ktNode*, ktNode*>> missing_pairs;
    std::set_difference(pair_cache.begin(), pair_cache.end(), pairs.begin(), pairs.end(), std::inserter(missing_pairs, missing_pairs.begin()));
    pair_cache = pairs;
    for(auto& p : missing_pairs) {
        auto it = col_listeners.find(p.first);
        if(it != col_listeners.end()) {
            it->second->onLeave(p.second);
        }
        it = col_listeners.find(p.second);
        if(it != col_listeners.end()) {
            it->second->onLeave(p.first);
        }
    }

    for(auto kv : rigid_bodies) {
        auto& rbinf = kv.second;
        auto rb = kv.first;
        gfxm::mat4 m(1.0f);
        btTransform btt = rbinf.bt_object->getWorldTransform();
        btt.getOpenGLMatrix((float*)&m);
        rb->getOwner()->getTransform()->setTransform(m);
    }
}

void DynamicsCtrl::debugDraw(DebugDraw& dd) {
    debugDrawer.setDD(&dd);
    world->debugDrawWorld();
}