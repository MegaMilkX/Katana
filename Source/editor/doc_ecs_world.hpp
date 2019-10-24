#ifndef DOC_ECS_WORLD_HPP
#define DOC_ECS_WORLD_HPP

#include "editor_document.hpp"
#include "../common/resource/ecs_world.hpp"

#include "../common/ecs/world.hpp"

#include "../common/renderer.hpp"
#include "../common/gui_viewport.hpp"
#include "../common/util/mesh_gen.hpp"

#include <btBulletCollisionCommon.h>
#include "../common/util/bullet_debug_draw.hpp"


class ecsTranslation : public ecsAttrib<ecsTranslation> {
public:
    gfxm::vec3 translation;
};
class ecsRotation : public ecsAttrib<ecsRotation> {
public:
    gfxm::quat rotation;
};
class ecsScale : public ecsAttrib<ecsScale> {
public:
    gfxm::vec3 scale;
};
class ecsVelocity : public ecsAttrib<ecsVelocity> {
public:
    gfxm::vec3 velo;
};
class ecsMass : public ecsAttrib<ecsMass> {
public:
    float mass = 1.0f;
};
class ecsCollisionShape : public ecsAttrib<ecsCollisionShape> {
public:
    ecsCollisionShape() {
        shape.reset(new btSphereShape(.5f));
    }
    std::unique_ptr<btCollisionShape> shape;
};

class ecsArchCollider : public ecsArchetype<ecsTranslation, ecsCollisionShape> {
public:
    ecsArchCollider(ecsEntity* ent)
    : ecsArchetype<ecsTranslation, ecsCollisionShape>(ent) {}

    std::shared_ptr<btCollisionObject> collision_object;
};
class ecsArchRigidBody : public ecsArchetype<ecsTranslation, ecsCollisionShape, ecsMass> {
public:
    ecsArchRigidBody(ecsEntity* ent)
    : ecsArchetype<ecsTranslation, ecsCollisionShape, ecsMass>(ent) {}

    std::shared_ptr<btRigidBody> rigid_body;
    btDefaultMotionState motion_state;
};



class ecsTestSystem : public ecsSystem<
    ecsArchetype<ecsTranslation, ecsVelocity>,
    ecsArchetype<ecsTranslation, ecsMass>
> {
public:
    void onFit(ecsArchetype<ecsTranslation, ecsVelocity>* ptr) {
        ptr->get<ecsTranslation>()->translation = gfxm::vec3(rand() % 100 * .1f, rand() % 100 * .1f, rand() % 100 * .1f);
    }

    void onUpdate() {
        for(auto& kv : get_archetype_map<ecsArchetype<ecsTranslation, ecsVelocity>>()) {
            gfxm::vec3& pos = kv.second->get<ecsTranslation>()->translation;
            gfxm::vec3& vel = kv.second->get<ecsVelocity>()->velo;

            vel.x += .01f;
            vel.y += .01f;
            float u = sinf(vel.x) * 10.0f;
            float v = cosf(vel.y) * 10.0f;
            
            pos = gfxm::vec3(
                (1 + v / 2 * cosf(u / 2)) * cosf(u), 
                (1 + v / 2 * cosf(u / 2)) * sinf(u), 
                v / 2 * sin(u / 2)
            );
        }
        for(auto& kv : get_archetype_map<ecsArchetype<ecsTranslation, ecsMass>>()) {
            //LOG(kv.first << ": Falling");
        }
    }
};

class ecsDynamicsSys : public ecsSystem<
    ecsArchCollider,
    ecsArchRigidBody
> {
    btDefaultCollisionConfiguration* collisionConf;
    btCollisionDispatcher* dispatcher;
    btDbvtBroadphase* broadphase;
    btSequentialImpulseConstraintSolver* constraintSolver;
    btDiscreteDynamicsWorld* world;

    BulletDebugDrawer2_OpenGL debugDrawer;
public:
    ecsDynamicsSys(DebugDraw* dd = 0) {
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
        debugDrawer.setDD(dd);
    }

    void onFit(ecsArchCollider* collider) {
        collider->collision_object.reset(new btCollisionObject());
        collider->collision_object->setCollisionShape(
            collider->get<ecsCollisionShape>()->shape.get()
        );

        world->addCollisionObject(collider->collision_object.get());
    }
    void onFit(ecsArchRigidBody* rb) {
        btVector3 local_inertia;
        rb->get<ecsCollisionShape>()->shape->calculateLocalInertia(
            rb->get<ecsMass>()->mass,
            local_inertia
        );
        btTransform btt, com;
        btt.setIdentity();
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

        world->addRigidBody(
            rb->rigid_body.get()
        );
    }

    void onUpdate() {
        for(auto& kv : get_archetype_map<ecsArchCollider>()) {
            gfxm::vec3 trans = kv.second->get<ecsTranslation>()->translation;

        }

        world->stepSimulation(1.0f/60.0f);

        for(auto& kv : get_archetype_map<ecsArchRigidBody>()) {
            
        }

        world->debugDrawWorld();
    }
};

class ecsRenderSystem : public ecsSystem<
    ecsArchetype<ecsTranslation, ecsVelocity>
> {
    DrawList draw_list;
    gl::IndexedMesh mesh;
public:
    ecsRenderSystem() {
        makeSphere(&mesh, 0.5f, 6);
    }

    void onFit(ecsArchetype<ecsTranslation, ecsVelocity>* object) {
    }

    void fillDrawList(DrawList& dl) {
        for(auto& kv : get_archetype_map<ecsArchetype<ecsTranslation, ecsVelocity>>()) {
            DrawCmdSolid cmd;
            cmd.indexOffset = 0;
            cmd.material = 0;
            cmd.object_ptr = 0;
            cmd.transform = gfxm::translate(gfxm::mat4(1.0f), kv.second->get<ecsTranslation>()->translation);
            cmd.vao = mesh.getVao();
            cmd.indexCount = mesh.getIndexCount();

            dl.solids.emplace_back(cmd);
        }
    }
};


class DocEcsWorld : public EditorDocumentTyped<EcsWorld> {
    ecsWorld world;
    ecsRenderSystem* renderSys;

    GuiViewport gvp;

public:
    DocEcsWorld() {
        world.addSystem<ecsTestSystem>();
        world.addSystem(new ecsDynamicsSys(&gvp.getDebugDraw()));
        renderSys = world.addSystem<ecsRenderSystem>();
        auto ent = world.createEntity();
        world.setAttrib<ecsTranslation>(ent);
        world.setAttrib<ecsVelocity>(ent);

        gvp.camMode(GuiViewport::CAM_ORBIT);
    }

    void onGui(Editor* ed, float dt) override {
        world.update();

        DrawList dl;
        renderSys->fillDrawList(dl);
        gvp.draw(dl);
    }
    void onGuiToolbox(Editor* ed) override {
        if(ImGui::Button("Add entity TRANS+VELO")) {
            auto ent = world.createEntity();
            world.setAttrib<ecsTranslation>(ent);
            world.setAttrib<ecsVelocity>(ent);
        }
        if(ImGui::Button("Add entity TRANS+COLLISIONSHAPE")) {
            auto ent = world.createEntity();
            world.setAttrib<ecsTranslation>(ent);
            world.setAttrib<ecsCollisionShape>(ent);
        }
        if(ImGui::Button("Add entity RIGID_BODY")) {
            auto ent = world.createEntity();
            world.setAttrib<ecsTranslation>(ent);
            world.setAttrib<ecsCollisionShape>(ent);
            world.setAttrib<ecsMass>(ent);
        }
    }
};
STATIC_RUN(DocEcsWorld) {
    regEditorDocument<DocEcsWorld>({ "ecsw" });
}

#endif
