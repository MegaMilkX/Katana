#ifndef SCENE_PHYSICS_WORLD_HPP
#define SCENE_PHYSICS_WORLD_HPP

#include "scene_component.hpp"
#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>
#include "../gfxm.hpp"
#include "../debug_draw.hpp"

class BulletDebugDrawer_OpenGL : public btIDebugDraw {
public:
	virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) 
	{
        DebugDraw::getInstance()->line(*(gfxm::vec3*)&from, *(gfxm::vec3*)&to, *(gfxm::vec3*)&color);
	}
	virtual void drawContactPoint(const btVector3 &, const btVector3 &, btScalar, int, const btVector3 &) {}
	virtual void reportErrorWarning(const char *) {}
	virtual void draw3dText(const btVector3 & pos, const char * text) {
        
    }
	virtual void setDebugMode(int p) {
		m = p;
	}
	int getDebugMode(void) const { return DBG_DrawWireframe | DBG_DrawAabb |  DBG_DrawContactPoints; }
	int m;
private:
    GLuint vao_handle = 0;
    GLuint vbuf = 0;
    gfxm::mat4 proj;
    gfxm::mat4 view;
};

class PhysicsWorld;
class ContactCallback : public btCollisionWorld::ContactResultCallback {
public:
    ContactCallback(PhysicsWorld* wrld, gfxm::vec3& pos)
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
    );

    bool hasHit() {
        return has_hit;
    }

    gfxm::vec3 hit_normal;
    gfxm::vec3 pos;
private:
    bool has_hit = false;
    PhysicsWorld* world = 0;
    
};

class ConvexSweepCallback : public btCollisionWorld::ConvexResultCallback {
public:
    ConvexSweepCallback(PhysicsWorld* wrld, float r, const gfxm::vec3& v_from, const gfxm::vec3& v_to, unsigned flags)
    : world(wrld), radius(r), from(v_from), to(v_to), flags(flags) {}

    virtual bool needsCollision(btBroadphaseProxy *proxy0) const {
        return true;
    }

    virtual btScalar addSingleResult(btCollisionWorld::LocalConvexResult &convexResult, bool normalInWorldSpace);

    bool hasHit() {
        return has_hit;
    }

    gfxm::vec3 closest_hit;
    gfxm::vec3 closest_hit_center;
    float closest_hit_fraction = 1.0f;
private:
    PhysicsWorld* world = 0;
    gfxm::vec3 from;
    gfxm::vec3 to;
    float radius;
    bool has_hit = false;
    unsigned flags = 0;
};

class PhysicsWorld : public SceneComponent {
public:
    PhysicsWorld() {
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
        world->setGravity(btVector3(0.0f, -10.0f, 0.0f));

        world->setDebugDrawer(&debugDrawer);
        /*
        btCollisionObject* co = new btCollisionObject();
        btBoxShape* box = new btBoxShape(btVector3(10.0f, 0.4f, 5.0f));
        co->setCollisionShape(box);
        world->addCollisionObject(co);
*/
        world->updateAabbs();
    }
    ~PhysicsWorld() {
        delete world;
        delete constraintSolver;
        delete broadphase;
        delete dispatcher;
        delete collisionConf;
    }

    bool rayCastClosestPoint(const gfxm::ray& r, gfxm::vec3& hit) {
        btCollisionWorld::ClosestRayResultCallback cb(
            btVector3(r.origin.x, r.origin.y, r.origin.z),
            btVector3(
                r.origin.x + r.direction.x,
                r.origin.y + r.direction.y,
                r.origin.z + r.direction.z
            )
        );
        world->rayTest(
            btVector3(r.origin.x, r.origin.y, r.origin.z),
            btVector3(
                r.origin.x + r.direction.x,
                r.origin.y + r.direction.y,
                r.origin.z + r.direction.z
            ),
            cb
        );

        if(cb.hasHit()) {
            hit = gfxm::vec3(
                cb.m_hitPointWorld.getX(),
                cb.m_hitPointWorld.getY(),
                cb.m_hitPointWorld.getZ()
            );
            return true;
        } else {
            return false;
        }
    }

    bool contactTest(btCollisionObject* co, gfxm::vec3& new_pos, unsigned flags) {
        ContactCallback cb(this, new_pos);
        world->contactTest(co, cb);
        if(cb.hasHit()) {
            new_pos = cb.pos;
            return true;
        }
        return false;
    }

    bool sphereSweepClosestHit(float radius, const gfxm::vec3& v_from, const gfxm::vec3& v_to, unsigned flags, gfxm::vec3& hit) {
        gfxm::mat4 from = 
            gfxm::translate(
                gfxm::mat4(1.0f), 
                v_from
            );
        gfxm::mat4 to = 
            gfxm::translate(
                gfxm::mat4(1.0f), 
                v_to
            );
        
        btSphereShape shape(radius);
        btVector3 vfrom(v_from.x, v_from.y, v_from.z);
        btVector3 vto(v_to.x, v_to.y, v_to.z);
        ConvexSweepCallback cb(this, radius, v_from, v_to, flags);

        world->convexSweepTest(
            &shape, 
            *(btTransform*)&from, 
            *(btTransform*)&to,
            cb
        );
        /*
        world->getDebugDrawer()->drawSphere(
            radius,
            *(btTransform*)&from,
            btVector3(1.0f, 1.0f, .0f)
        );
        world->getDebugDrawer()->drawSphere(
            radius,
            *(btTransform*)&to,
            btVector3(1.0f, 0.5f, .0f)
        );*/
        if(cb.hasHit()) {
            hit = cb.closest_hit;
        }

        return cb.hasHit();
    }

    bool sphereSweep(float radius, const gfxm::vec3& v_from, const gfxm::vec3& v_to, unsigned flags) {
        gfxm::mat4 from = 
            gfxm::translate(
                gfxm::mat4(1.0f), 
                v_from
            );
        gfxm::mat4 to = 
            gfxm::translate(
                gfxm::mat4(1.0f), 
                v_to
            );
        
        btSphereShape shape(radius);
        btVector3 vfrom(v_from.x, v_from.y, v_from.z);
        btVector3 vto(v_to.x, v_to.y, v_to.z);
        ConvexSweepCallback cb(this, radius, v_from, v_to, flags);

        world->convexSweepTest(
            &shape, 
            *(btTransform*)&from, 
            *(btTransform*)&to,
            cb
        );
        /*
        world->getDebugDrawer()->drawSphere(
            radius,
            *(btTransform*)&from,
            btVector3(1.0f, 1.0f, .0f)
        );
        world->getDebugDrawer()->drawSphere(
            radius,
            *(btTransform*)&to,
            btVector3(1.0f, 0.5f, .0f)
        );*/
        if(cb.hasHit()) {
            
        }

        return cb.hasHit();
    }

    void update(float dt) {
        world->stepSimulation(dt);
    }

    btCollisionWorld* getBtWorld() {
        return world;
    }

    void debugDraw() {
        world->debugDrawWorld();
    }

private:
    btDefaultCollisionConfiguration* collisionConf;
    btCollisionDispatcher* dispatcher;
    btDbvtBroadphase* broadphase;
	//btCollisionWorld* world;
    btSequentialImpulseConstraintSolver* constraintSolver;
    btDiscreteDynamicsWorld* world;

    BulletDebugDrawer_OpenGL debugDrawer;
};

inline btScalar ContactCallback::addSingleResult(
    btManifoldPoint& cp, 
    const btCollisionObjectWrapper* colObj0Wrap, 
    int partId0, 
    int index0, 
    const btCollisionObjectWrapper* colObj1Wrap, 
    int partId1, 
    int index1
) {
    if(cp.getDistance() > 0.0f) {
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
        cp.m_positionWorldOnB + cp.m_normalWorldOnB,
        btVector3(1.0f, .0f, .0f)
    );

    btVector3 nrm = cp.m_positionWorldOnB - cp.m_positionWorldOnA;

    pos += gfxm::vec3(
        nrm.getX(),
        nrm.getY(),
        nrm.getZ()
    ) * std::min(gfxm::sqrt(-cp.getDistance()), 1.0f);

    has_hit = true;

    return 1.0f;
}

#endif
