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

#include "../common/ecs/attribs/transform.hpp"

class ecsName : public ecsAttrib<ecsName> {
public:
    std::string name;
    virtual void onGui(ecsWorld* world, entity_id ent) {
        char buf[256];
        memset(buf, 0, sizeof(buf));
        memcpy(buf, name.c_str(), name.size());
        if(ImGui::InputText("name", buf, sizeof(buf))) {
            name = buf;
        }
    }
};

class ecsVelocity : public ecsAttrib<ecsVelocity> {
public:
    gfxm::vec3 velo;
    virtual void onGui(ecsWorld* world, entity_id ent) {
        ImGui::DragFloat3("velo", (float*)&velo, 0.01f);
    }
};
class ecsMass : public ecsAttrib<ecsMass> {
public:
    float mass = 1.0f;
    virtual void onGui(ecsWorld* world, entity_id ent) {
        if(ImGui::DragFloat("mass", &mass, 0.01f)) {
            world->updateAttrib(ent, *this);
        }
    }
};
class ecsCollisionShape : public ecsAttrib<ecsCollisionShape> {
public:
    ecsCollisionShape() {
        shape.reset(new btSphereShape(.5f));
    }
    std::shared_ptr<btCollisionShape> shape;
};

class ecsArchCollider : public ecsArchetype<ecsTransform, ecsCollisionShape, ecsExclude<ecsMass>> {
public:
    ecsArchCollider(ecsEntity* ent)
    : ecsArchetype<ecsTransform, ecsCollisionShape, ecsExclude<ecsMass>>(ent) {}

    void onAttribUpdate(ecsCollisionShape* shape) override {
        world->removeCollisionObject(collision_object.get());
        collision_object->setCollisionShape(shape->shape.get());
        world->addCollisionObject(collision_object.get());
        LOG("Collider shape changed");
    }

    btCollisionWorld* world = 0;
    std::shared_ptr<btCollisionObject> collision_object;
};
class ecsArchRigidBody : public ecsArchetype<ecsTransform, ecsCollisionShape, ecsMass> {
public:
    ecsArchRigidBody(ecsEntity* ent)
    : ecsArchetype<ecsTransform, ecsCollisionShape, ecsMass>(ent) {}

    void onAttribUpdate(ecsTransform* t) override {
        //auto& transform = rigid_body->getWorldTransform();        
        btTransform transform;
        const gfxm::mat4& world_transform = t->getWorldTransform();
        //transform.setFromOpenGLMatrix((float*)&world_transform);
        
        auto p = t->getWorldPosition();
        auto r = t->getWorldRotation();
        auto s = t->getWorldScale();
        transform.setIdentity();
        transform.setOrigin(btVector3(p.x, p.y, p.z));
        transform.setRotation(btQuaternion(r.x, r.y, r.z, r.w));
        
        //LOG(world_transform);
        rigid_body->setWorldTransform(transform);
        world->updateSingleAabb(rigid_body.get());
    }

    void onAttribUpdate(ecsMass* m) override {
        rigid_body->setMassProps(m->mass, btVector3(.0f, .0f, .0f));
    }

    void onAttribUpdate(ecsCollisionShape* shape) override {
        world->removeRigidBody(rigid_body.get());
        rigid_body->setCollisionShape(shape->shape.get());
        world->addRigidBody(rigid_body.get());
        LOG("RigidBody shape changed");
    }

    btDiscreteDynamicsWorld* world = 0;
    std::shared_ptr<btRigidBody> rigid_body;
    btDefaultMotionState motion_state;
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
        collider->world = world;
        collider->collision_object.reset(new btCollisionObject());
        collider->collision_object->setCollisionShape(
            collider->get<ecsCollisionShape>()->shape.get()
        );

        world->addCollisionObject(collider->collision_object.get());
    }
    void onUnfit(ecsArchCollider* collider) {
        world->removeCollisionObject(
            collider->collision_object.get()
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
    void onUnfit(ecsArchRigidBody* rb) {
        world->removeRigidBody(
            rb->rigid_body.get()
        );
    }

    void onUpdate() {
        for(auto& kv : get_archetype_map<ecsArchCollider>()) {

        }

        world->stepSimulation(1.0f/60.0f);

        for(auto& kv : get_archetype_map<ecsArchRigidBody>()) {
            auto& t = kv.second->rigid_body->getWorldTransform();
            btVector3 btv3 = t.getOrigin();
            btQuaternion btq = t.getRotation();
            auto transform = kv.second->get<ecsTransform>();
            transform->setPosition(btv3.getX(), btv3.getY(), btv3.getZ());
            transform->setRotation(btq.getX(), btq.getY(), btq.getZ(), btq.getW());
        }

        world->debugDrawWorld();
    }
};

class ecsRenderSystem : public ecsSystem<
    ecsArchetype<ecsTransform>
> {
    DrawList draw_list;
    gl::IndexedMesh mesh;
public:
    ecsRenderSystem() {
        makeSphere(&mesh, 0.5f, 6);
    }

    void onFit(ecsArchetype<ecsTransform>* object) {
    }

    void fillDrawList(DrawList& dl) {
        for(auto& kv : get_archetype_map<ecsArchetype<ecsTransform>>()) {
            DrawCmdSolid cmd;
            cmd.indexOffset = 0;
            cmd.material = 0;
            cmd.object_ptr = 0;
            cmd.transform = kv.second->get<ecsTransform>()->getWorldTransform();
            cmd.vao = mesh.getVao();
            cmd.indexCount = mesh.getIndexCount();

            dl.solids.emplace_back(cmd);
        }
    }
};

class ecsSceneGraphMonitorSys : public ecsSystem<
    ecsArchetype<ecsTransform>
> {
    std::set<ecsArchetype<ecsTransform>*> root_transforms;
public:
    void onFit(ecsArchetype<ecsTransform>* object) override {
        root_transforms.insert(object);
    }
    void onUnfit(ecsArchetype<ecsTransform>* object) override {
        root_transforms.erase(object);
    }

    void onUpdate() {
        
    }

    void onGui() {
        for(auto o : root_transforms) {
            ImGui::Selectable("ENTITY_NAME", false);
        }
    }
};


class DocEcsWorld : public EditorDocumentTyped<EcsWorld> {
    ecsWorld world;
    ecsRenderSystem* renderSys;
    ecsSceneGraphMonitorSys* sceneGraphMonitor;

    GuiViewport gvp;

public:
    DocEcsWorld() {
        sceneGraphMonitor = new ecsSceneGraphMonitorSys();
        world.addSystem(sceneGraphMonitor);
        world.addSystem(new ecsDynamicsSys(&gvp.getDebugDraw()));
        renderSys = world.addSystem<ecsRenderSystem>();
        auto ent = world.createEntity();
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
        sceneGraphMonitor->onGui();

        if(ImGui::Button("Add entity TRANS+VELO")) {
            auto ent = world.createEntity();
            world.setAttrib<ecsName>(ent);
            world.setAttrib<ecsTransform>(ent);
            world.setAttrib<ecsVelocity>(ent);
        }
        if(ImGui::Button("Add entity TRANS+COLLISIONSHAPE")) {
            auto ent = world.createEntity();
            world.setAttrib<ecsTransform>(ent);
            world.setAttrib<ecsCollisionShape>(ent);

            ecsCollisionShape shape;
            shape.shape.reset(new btStaticPlaneShape(btVector3(.0f, 1.0f, .0f), .0f));
            world.updateAttrib(ent, shape);
        }
        if(ImGui::Button("Add entity RIGID_BODY")) {
            auto ent = world.createEntity();
            world.setAttrib<ecsName>(ent);
            world.setAttrib<ecsTransform>(ent);
            world.setAttrib<ecsCollisionShape>(ent);
            world.setAttrib<ecsMass>(ent);
        }

        static entity_id selected_ent = 0;
        for(auto& eid : world.getEntities()) {
            if(ImGui::Selectable(MKSTR("entity###" << eid).c_str(), selected_ent == eid)) {
                selected_ent = eid;
            }
        }

        for(uint8_t i = 0; i < 64; ++i) {
            auto attrib = world.getAttribPtr(selected_ent, i);
            if(attrib) {
                attrib->onGui(&world, selected_ent);
            }
        }
    }
};
STATIC_RUN(DocEcsWorld) {
    regEditorDocument<DocEcsWorld>({ "ecsw" });
}

#endif
