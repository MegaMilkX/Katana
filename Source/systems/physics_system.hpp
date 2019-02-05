#ifndef PHYSICS_SYSTEM_HPP
#define PHYSICS_SYSTEM_HPP

#include "scene.hpp"
#include "../components/collider.hpp"

#include <btBulletDynamicsCommon.h>

#include "../gl/shader_program.h"
#include "../shader_factory.hpp"

#include "../debug_draw.hpp"

class BulletDebugDrawer_OpenGL2 : public btIDebugDraw {
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
	int getDebugMode(void) const { return 1; }
	int m;
private:
    GLuint vao_handle = 0;
    GLuint vbuf = 0;
    gfxm::mat4 proj;
    gfxm::mat4 view;
};

class PhysicsSystem
: public ISceneProbe<Collider> {
public:
    PhysicsSystem() {
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

        btCollisionObject* co = new btCollisionObject();
        btBoxShape* box = new btBoxShape(btVector3(10.0f, 0.5f, 5.0f));
        co->setCollisionShape(box);
        world->addCollisionObject(co);

        world->updateAabbs();
    }
    ~PhysicsSystem() {
        delete world;
        delete constraintSolver;
        delete broadphase;
        delete dispatcher;
        delete collisionConf;
    }

    virtual void onCreateComponent(Collider* c) {
        world->addCollisionObject(c->collision_object.get());
    }
    virtual void onRemoveComponent(Collider* c) {
        world->removeCollisionObject(c->collision_object.get());
    }

    void setScene(Scene* scn) {
        if(scene) {
            scene->removeProbe<Collider>();
        }
        scene = scn;
        if(scene) {
            scene->setProbe<Collider>(this);
        }
    }

    void debugDraw() {
        /*
        glDepthFunc(GL_LEQUAL);
        debugDrawer.setProjView(proj, view);
        */
        world->debugDrawWorld();
    }

    void update(float dt) {

    }
private:
    Scene* scene = 0;

    btDefaultCollisionConfiguration* collisionConf;
    btCollisionDispatcher* dispatcher;
    btDbvtBroadphase* broadphase;
	//btCollisionWorld* world;
    btSequentialImpulseConstraintSolver* constraintSolver;
    btDiscreteDynamicsWorld* world;

    BulletDebugDrawer_OpenGL2 debugDrawer;
};

#endif
