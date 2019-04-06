#include "dynamics_ctrl.hpp"

#include "../game_object.hpp"

#include "../../components/collision_shape.hpp"
#include "../../components/collision_object.hpp"
#include "../../components/rigid_body.hpp"
#include "../../components/ghost_object.hpp"

class ConvexSweepCallback_ : public btCollisionWorld::ConvexResultCallback {
public:
    ConvexSweepCallback_(DynamicsCtrl* wrld, float r, const gfxm::vec3& v_from, const gfxm::vec3& v_to, unsigned flags)
    : world(wrld), radius(r), from(v_from), to(v_to), flags(flags) {}

    virtual bool needsCollision(btBroadphaseProxy *proxy0) const {
        return true;
    }

    virtual btScalar addSingleResult(btCollisionWorld::LocalConvexResult &convexResult, bool normalInWorldSpace) {
        if(convexResult.m_hitCollisionObject->getInternalType() == btCollisionObject::CO_GHOST_OBJECT) {
            return 0.0f;
        }
        // TODO: Check collision group mask here

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
    unsigned flags = 0;
};

class ContactAdjustCallback_ : public btCollisionWorld::ContactResultCallback {
public:
    ContactAdjustCallback_(DynamicsCtrl* wrld, gfxm::vec3& pos)
    : world(wrld), pos(pos) {

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
}
DynamicsCtrl::~DynamicsCtrl() {
    delete world;
    delete constraintSolver;
    delete broadphase;
    delete dispatcher;
    delete collisionConf;
}

bool DynamicsCtrl::getAdjustedPosition(btCollisionObject* o, gfxm::vec3& pos) {
    ContactAdjustCallback_ cb(this, pos);
    world->contactTest(o, cb);
    if(cb.hasHit()) {
        pos = cb.pos;
        return true;
    }
    return false;
}

void DynamicsCtrl::sweepTest(GhostObject* go, const gfxm::mat4& from, const gfxm::mat4& to) {
    btTransform tfrom;
    btTransform tto;
    tfrom.setFromOpenGLMatrix((float*)&from);
    tto.setFromOpenGLMatrix((float*)&to);

    ConvexSweepCallback_ cb(this, .2f, from[3], to[3], 0);
    
    world->convexSweepTest(
        (btConvexShape*)go->getBtObject()->getCollisionShape(),
        tfrom, tto, cb
    );
}

bool DynamicsCtrl::sweepSphere(float radius, const gfxm::vec3& from, const gfxm::vec3& to, gfxm::vec3& hit) {
    btSphereShape shape(radius);
    btTransform tfrom;
    btTransform tto;
    tfrom.setIdentity();
    tto.setIdentity();
    tfrom.setOrigin(btVector3(from.x, from.y, from.z));
    tto.setOrigin(btVector3(to.x, to.y, to.z));
    ConvexSweepCallback_ cb(this, radius, from, to, 0);
    
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
    for(auto g : ghosts) {
        if(g->getOwner()->getTransform()->_isFrameDirty()) {
            g->_updateTransform();
        }
    }
    for(auto c : colliders) {
        if(c->getOwner()->getTransform()->_isFrameDirty()) {
            c->_updateTransform();
        }
    }
    world->stepSimulation(dt);
    
    for(auto r : bodies) {
        r->_updateTransformFromBody();
    }
}

void DynamicsCtrl::debugDraw(DebugDraw& dd) {
    debugDrawer.setDD(&dd);
    world->debugDrawWorld();
    
    for(auto s : shapes) {
        s->debugDraw(world->getDebugDrawer());
    }
}

void DynamicsCtrl::_addCollider(CmCollisionObject* col) {
    colliders.insert(col);
    if(!col->getBtObject()) return;
    world->addCollisionObject(col->getBtObject());
}
void DynamicsCtrl::_removeCollider(CmCollisionObject* col) {
    colliders.erase(col);
    if(!col->getBtObject()) return;
    world->removeCollisionObject(col->getBtObject());
}

void DynamicsCtrl::_addGhost(GhostObject* col) {
    ghosts.insert(col);
    if(!col->getBtObject()) return;
    world->addCollisionObject(col->getBtObject());
}
void DynamicsCtrl::_removeGhost(GhostObject* col) {
    ghosts.erase(col);
    if(!col->getBtObject()) return;
    world->removeCollisionObject(col->getBtObject());
}

void DynamicsCtrl::_addRigidBody(CmRigidBody* col) {
    bodies.insert(col);
    if(!col->getBtBody()) return;
    world->addRigidBody(col->getBtBody());
}
void DynamicsCtrl::_removeRigidBody(CmRigidBody* col) {
    bodies.erase(col);
    if(!col->getBtBody()) return;
    world->removeRigidBody(col->getBtBody());
}

void DynamicsCtrl::_addCollisionShape(CmCollisionShape* s) {
    shapes.insert(s);
}
void DynamicsCtrl::_removeCollisionShape(CmCollisionShape* s) {
    shapes.erase(s);
}

void DynamicsCtrl::_shapeChanged(CmCollisionShape* s) {
    GameObject* o = s->getOwner();
    auto cobj = o->find<CmCollisionObject>();
    if(cobj) {
        cobj->_shapeChanged();
    }
    auto robj = o->find<CmRigidBody>();
    if(robj) {
        robj->_shapeChanged();
    }
    auto gobj = o->find<GhostObject>();
    if(gobj) {
        gobj->_shapeChanged();
    }
}