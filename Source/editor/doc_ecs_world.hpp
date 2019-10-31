#ifndef DOC_ECS_WORLD_HPP
#define DOC_ECS_WORLD_HPP

#include "editor_document.hpp"
#include "../common/resource/ecs_world.hpp"

#include "../common/ecs/world.hpp"

#include "../common/renderer.hpp"
#include "../common/gui_viewport.hpp"
#include "../common/util/mesh_gen.hpp"


#include "../common/util/bullet_debug_draw.hpp"

#include "../common/ecs/attribs/base_attribs.hpp"

class ecsArchCollider : public ecsArchetype<ecsTransform, ecsCollisionShape, ecsExclude<ecsMass>> {
public:
    ecsArchCollider(ecsEntity* ent)
    : ecsArchetype<ecsTransform, ecsCollisionShape, ecsExclude<ecsMass>>(ent) {}

    void onAttribUpdate(ecsCollisionShape* shape) override {
        world->removeCollisionObject(collision_object.get());
        collision_object->setCollisionShape(shape->shape.get());
        world->addCollisionObject(collision_object.get());
        //LOG("Collider shape changed");
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
        //LOG("RigidBody shape changed");
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
            auto& matrix = kv.second->get<ecsTransform>()->getWorldTransform();
            kv.second->collision_object->getWorldTransform().setFromOpenGLMatrix((float*)&matrix);
            world->updateSingleAabb(kv.second->collision_object.get());
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
    ecsArchetype<ecsTransform, ecsMeshes>
> {
    DrawList draw_list;
    gl::IndexedMesh mesh;
public:
    ecsRenderSystem() {
        makeSphere(&mesh, 0.5f, 6);
    }

    void onFit(ecsArchetype<ecsTransform, ecsMeshes>* object) {
    }

    void fillDrawList(DrawList& dl) {
        for(auto& kv : get_archetype_map<ecsArchetype<ecsTransform, ecsMeshes>>()) {
            for(auto& seg : kv.second->get<ecsMeshes>()->segments) {
                if(!seg.mesh) continue;
                
                GLuint vao = seg.mesh->mesh.getVao();
                Material* mat = seg.material.get();
                gfxm::mat4 transform = kv.second->get<ecsTransform>()->getWorldTransform();
                size_t indexOffset = seg.mesh->submeshes.size() > 0 ? seg.mesh->submeshes[seg.submesh_index].indexOffset : 0;
                size_t indexCount = seg.mesh->submeshes.size() > 0 ? (seg.mesh->submeshes[seg.submesh_index].indexCount) : seg.mesh->mesh.getIndexCount();
                
                if(!seg.skin_data) {    // Static mesh
                    DrawCmdSolid s;
                    s.vao = vao;
                    s.material = mat;
                    s.indexCount = indexCount;
                    s.indexOffset = indexOffset;
                    s.transform = transform;
                    //s.object_ptr = getOwner();
                    dl.solids.emplace_back(s);
                } else {                // Skinned mesh
                    std::vector<gfxm::mat4> bone_transforms;
                    for(auto t : seg.skin_data->bone_nodes) {
                        if(t) {
                            bone_transforms.emplace_back(t->getTransform()->getWorldTransform());
                        } else {
                            bone_transforms.emplace_back(gfxm::mat4(1.0f));
                        }
                    }

                    DrawCmdSkin s;
                    s.vao = vao;
                    s.material = mat;
                    s.indexCount = indexCount;
                    s.indexOffset = indexOffset;
                    s.transform = transform;
                    s.bone_transforms = bone_transforms;
                    s.bind_transforms = seg.skin_data->bind_transforms;
                    //s.object_ptr = getOwner();
                    dl.skins.emplace_back(s);
                }
            }
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
            //ImGui::Selectable("ENTITY_NAME", false);
        }
    }
};


#include "../common/ecs/util/assimp_to_ecs_world.hpp"

class DocEcsWorld : public EditorDocumentTyped<EcsWorld> {
    ecsWorld world;
    entity_id selected_ent = 0;
    ecsRenderSystem* renderSys;
    ecsSceneGraphMonitorSys* sceneGraphMonitor;

    GuiViewport gvp;

public:
    DocEcsWorld() {
        regEcsAttrib<ecsName>("Name");
        regEcsAttrib<ecsTransform>("Transform");
        regEcsAttrib<ecsTransformTree>("TransformTree");
        regEcsAttrib<ecsCollisionShape>("CollisionShape", "Collision");
        regEcsAttrib<ecsMass>("Mass", "Physics");
        regEcsAttrib<ecsMeshes>("Meshes", "Rendering");

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

        ImGui::Columns(2);

        gvp.draw(dl);
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_RESOURCE")) {
                ResourceNode* node = *(ResourceNode**)payload->Data;
                LOG("Payload received: " << node->getFullName());
                if(has_suffix(node->getName(), ".fbx")) {
                    assimpImportEcsScene(&world, node->getFullName().c_str());
                }
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::NextColumn();
        //ImGui::SetColumnWidth(1, 150);
        
        ImGui::BeginChild(ImGui::GetID("0"));
        ImGui::PushItemWidth(-1);
        if(ImGui::ListBoxHeader("###OBJECT_LIST", ImVec2(0, 300))) {
            for(auto& eid : world.getEntities()) {
                ecsName* name_attrib = world.findAttrib<ecsName>(eid);
                std::string entity_name = "[anonymous]";
                if(name_attrib) {
                    if(name_attrib->name.size()) {
                        entity_name = name_attrib->name;
                    } else {
                        entity_name = "[empty_name]";
                    }
                }
                if(ImGui::Selectable(MKSTR(entity_name << "###" << eid).c_str(), selected_ent == eid)) {
                    selected_ent = eid;
                }
            }    
                
            ImGui::ListBoxFooter();
        }
        ImGui::PopItemWidth();
        if(ImGui::SmallButton(ICON_MDI_PLUS)) {
            selected_ent = world.createEntity();
        }
        ImGui::SameLine();
        if(ImGui::SmallButton(ICON_MDI_MINUS)) {
            world.removeEntity(selected_ent);
        }
        ImGui::EndChild();

        ImGui::Columns(1);
    }
    void onGuiToolbox(Editor* ed) override {
        sceneGraphMonitor->onGui();

        if(!world.getEntity(selected_ent)) {
            ImGui::Text("Nothing selected");
            return;
        } else {
            ImGui::Text(MKSTR("UID: " << selected_ent).c_str());
        }

        ImGui::PushItemWidth(-1);
        if(ImGui::BeginCombo("###ADD_ATTRIBUTE", "Add attribute...")) {
            for(auto& it : getEcsAttribTypeLib().getTable()) {
                if(it.first == "") {
                    for(auto& attr_id : it.second) {
                        auto inf = getEcsAttribTypeLib().get_info(attr_id);
                        if(ImGui::Selectable(inf->name.c_str())) {
                            world.setAttrib(selected_ent, attr_id);
                        }
                    }
                } else {
                    bool open = ImGui::TreeNode(it.first.c_str());
                    if(open) {
                        for(auto& attr_id : it.second) {
                            auto inf = getEcsAttribTypeLib().get_info(attr_id);
                            if(ImGui::Selectable(inf->name.c_str())) {
                                world.setAttrib(selected_ent, attr_id);
                            }
                        }

                        ImGui::TreePop();
                    }
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        for(uint8_t i = 0; i < 64; ++i) {
            auto attrib = world.getAttribPtr(selected_ent, i);
            if(!attrib) {
                continue;
            }
            auto inf = getEcsAttribTypeLib().get_info(attrib->get_id());
            const std::string& name = inf->name;
            bool exists = true;
            if(ImGui::CollapsingHeader(MKSTR( name ).c_str(), &exists, ImGuiTreeNodeFlags_DefaultOpen)) {
                attrib->onGui(&world, selected_ent);
            }
            if(!exists) {
                world.removeAttrib(selected_ent, i);
            }
        }
    }
};
STATIC_RUN(DocEcsWorld) {
    regEditorDocument<DocEcsWorld>({ "ecsw" });
}

#endif
