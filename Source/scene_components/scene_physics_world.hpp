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

class ContactCallback : public btCollisionWorld::ContactResultCallback {
public:
    virtual btScalar addSingleResult(
        btManifoldPoint& cp, 
        const btCollisionObjectWrapper* colObj0Wrap, 
        int partId0, 
        int index0, 
        const btCollisionObjectWrapper* colObj1Wrap, 
        int partId1, 
        int index1
    ) {

    }

    bool hasHit() {
        return has_hit;
    }
private:
    bool has_hit = false;
};

class ConvexSweepCallback : public btCollisionWorld::ConvexResultCallback {
public:
    ConvexSweepCallback(gfxm::vec3 delta, gfxm::vec3 initial_translation)
    : delta(delta), initial_translation(initial_translation),
    closestResult(0, 0, btVector3(0,0,0), btVector3(0,0,0), btScalar(1.0f)) {
        
    }

    virtual bool needsCollision(btBroadphaseProxy *proxy0) const {
        return true;
    }

    virtual btScalar addSingleResult(btCollisionWorld::LocalConvexResult &convexResult, bool normalInWorldSpace) {
        if(!convexResult.m_hitCollisionObject->isStaticObject()) {
            return 0.0f;
        }

        return convexResult.m_hitFraction;
    }

    gfxm::vec3 hitNormal;

    btCollisionWorld::LocalConvexResult closestResult;
private:
    gfxm::vec3 delta;
    gfxm::vec3 initial_translation;
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

    bool contactTest(btCollisionObject* co) {
        ContactCallback cb;
        world->contactTest(co, cb);
        if(cb.hasHit()) {

        }
    }

    bool sphereSweep(float radius, const gfxm::mat4& from, const gfxm::mat4& to, gfxm::vec3& normal) {
        btSphereShape shape(radius);
        btVector3 vfrom(from[3].x, from[3].y, from[3].z);
        btVector3 vto(to[3].x, to[3].y, to[3].z);
        btCollisionWorld::ClosestConvexResultCallback cb(
            vfrom, vto
        );

        world->convexSweepTest(
            &shape, 
            *(btTransform*)&from, 
            *(btTransform*)&to,
            cb
        );

        world->getDebugDrawer()->drawSphere(
            radius,
            *(btTransform*)&from,
            btVector3(1.0f, 1.0f, .0f)
        );
        world->getDebugDrawer()->drawSphere(
            radius,
            *(btTransform*)&to,
            btVector3(1.0f, 0.5f, .0f)
        );
        if(cb.hasHit()) {
            gfxm::vec3 hit_pt_world(
                cb.m_hitPointWorld.getX(),
                cb.m_hitPointWorld.getY(),
                cb.m_hitPointWorld.getZ()
            );

            world->getDebugDrawer()->drawLine(
                cb.m_hitPointWorld,
                cb.m_hitPointWorld + cb.m_hitNormalWorld,
                btVector3(.0f, .0f, 1.0f)
            );

            world->getDebugDrawer()->drawContactPoint(
                cb.m_hitPointWorld,
                cb.m_hitNormalWorld,
                1.0f, 0,
                btVector3(.0f, .0f, 1.0f)
            );

            normal = gfxm::vec3(
                cb.m_hitNormalWorld.getX(),
                cb.m_hitNormalWorld.getY(),
                cb.m_hitNormalWorld.getZ()
            );
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

#endif
