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

class ecsArchCollider : public ecsTuple<ecsWorldTransform, ecsCollisionShape, ecsExclude<ecsMass>> {
public:
    void onAttribUpdate(ecsCollisionShape* shape) override {
        world->removeCollisionObject(collision_object.get());
        collision_object->setCollisionShape(shape->shape.get());
        world->addCollisionObject(collision_object.get());
        //LOG("Collider shape changed");
    }

    btCollisionWorld* world = 0;
    std::shared_ptr<btCollisionObject> collision_object;
};
class ecsArchRigidBody : public ecsTuple<ecsWorldTransform, ecsCollisionShape, ecsMass> {
public:
    void onAttribUpdate(ecsWorldTransform* t) override {
        //auto& transform = rigid_body->getWorldTransform();        
        btTransform transform;
        const gfxm::mat4& world_transform = t->transform;
        transform.setFromOpenGLMatrix((float*)&world_transform);
        
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

        world->setGravity(btVector3(0.0f, -10.0f, 0.0f));

        world->setDebugDrawer(&debugDrawer);
    }

    void setDebugDraw(DebugDraw* dd) {
        debugDrawer.setDD(dd);
    }

    void onFit(ecsArchCollider* collider) {
        collider->world = world;
        collider->collision_object.reset(new btCollisionObject());
        collider->collision_object->setCollisionShape(
            collider->get<ecsCollisionShape>()->shape.get()
        );

        btTransform btt;
        btt.setFromOpenGLMatrix((float*)&collider->get<ecsWorldTransform>()->transform);
        collider->collision_object->setWorldTransform(btt);

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
        btt.setFromOpenGLMatrix((float*)&rb->get<ecsWorldTransform>()->transform);
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
        for(auto& a : get_array<ecsArchCollider>()) {
            auto& matrix = a->get<ecsWorldTransform>()->transform;
            a->collision_object->getWorldTransform().setFromOpenGLMatrix((float*)&matrix);
            world->updateSingleAabb(a->collision_object.get());
        }

        world->stepSimulation(1.0f/60.0f);

        for(auto& a : get_array<ecsArchRigidBody>()) {
            auto& t = a->rigid_body->getWorldTransform();
            btVector3 btv3 = t.getOrigin();
            btQuaternion btq = t.getRotation();
            auto& transform = a->get<ecsWorldTransform>()->transform;
            t.getOpenGLMatrix((float*)&transform);
        }

        world->debugDrawWorld();
    }
};


class ecsRenderSubSystem;
class ecsTupleSubSceneRenderable : public ecsTuple<ecsWorldTransform, ecsSubScene, ecsTagSubSceneRender> {
public:
    ecsRenderSubSystem* sub_system = 0;

    void onAttribUpdate(ecsSubScene* scn) override {
        if(!scn->getWorld()) {
            sub_system = 0;
        } else {
            sub_system = scn->getWorld()->getSystem<ecsRenderSubSystem>();
        }
    }
};

class ecsRenderSubSystem : public ecsSystem<
    ecsTuple<ecsWorldTransform, ecsMeshes>,
    ecsTupleSubSceneRenderable
> {
public:
    void fillDrawList(DrawList& dl, const gfxm::mat4& root_transform) {
        for(auto& a : get_array<ecsTuple<ecsWorldTransform, ecsMeshes>>()) {
            for(auto& seg : a->get<ecsMeshes>()->segments) {
                if(!seg.mesh) continue;
                
                GLuint vao = seg.mesh->mesh.getVao();
                Material* mat = seg.material.get();
                gfxm::mat4 transform = a->get<ecsWorldTransform>()->transform;
                size_t indexOffset = seg.mesh->submeshes.size() > 0 ? seg.mesh->submeshes[seg.submesh_index].indexOffset : 0;
                size_t indexCount = seg.mesh->submeshes.size() > 0 ? (seg.mesh->submeshes[seg.submesh_index].indexCount) : seg.mesh->mesh.getIndexCount();
                
                if(!seg.skin_data) {    // Static mesh
                    DrawCmdSolid s;
                    s.vao = vao;
                    s.material = mat;
                    s.indexCount = indexCount;
                    s.indexOffset = indexOffset;
                    s.transform = root_transform * transform;
                    //s.object_ptr = getOwner();
                    dl.solids.emplace_back(s);
                } else {                // Skinned mesh
                    std::vector<gfxm::mat4> bone_transforms;
                    for(auto t : seg.skin_data->bone_nodes) {
                        if(t) {
                            bone_transforms.emplace_back(root_transform * t->transform);
                        } else {
                            bone_transforms.emplace_back(root_transform * gfxm::mat4(1.0f));
                        }
                    }

                    DrawCmdSkin s;
                    s.vao = vao;
                    s.material = mat;
                    s.indexCount = indexCount;
                    s.indexOffset = indexOffset;
                    s.transform = root_transform * transform;
                    s.bone_transforms = bone_transforms;
                    s.bind_transforms = seg.skin_data->bind_transforms;
                    //s.object_ptr = getOwner();
                    dl.skins.emplace_back(s);
                }
            }
        }
    }
};

class ecsRenderSystem : public ecsSystem<
    ecsTuple<ecsWorldTransform, ecsMeshes>,
    ecsTupleSubSceneRenderable
> {
    DrawList draw_list;
public:
    ecsRenderSystem() {}

    void onFit(ecsTupleSubSceneRenderable* r) {
        if(r->get<ecsSubScene>()->getWorld()) {
            ecsRenderSubSystem* sys = r->get<ecsSubScene>()->getWorld()->getSystem<ecsRenderSubSystem>();
            r->sub_system = sys;
        }
    }

    void fillDrawList(DrawList& dl) {
        for(auto& a : get_array<ecsTupleSubSceneRenderable>()) {
            a->sub_system->fillDrawList(dl, a->get<ecsWorldTransform>()->transform);
        }

        for(auto& a : get_array<ecsTuple<ecsWorldTransform, ecsMeshes>>()) {
            for(auto& seg : a->get<ecsMeshes>()->segments) {
                if(!seg.mesh) continue;
                
                GLuint vao = seg.mesh->mesh.getVao();
                Material* mat = seg.material.get();
                gfxm::mat4 transform = a->get<ecsWorldTransform>()->transform;
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
                            bone_transforms.emplace_back(t->transform);
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

    void fillDrawList(DrawList& dl, entity_id entity_filter) {
        for(auto& a : get_array<ecsTupleSubSceneRenderable>()) {
            if(a->getEntityUid() == entity_filter) {
                a->sub_system->fillDrawList(dl, a->get<ecsWorldTransform>()->transform);
                break;
            }
        }

        for(auto& a : get_array<ecsTuple<ecsWorldTransform, ecsMeshes>>()) {
            if(a->getEntityUid() != entity_filter) {
                continue;
            }
            for(auto& seg : a->get<ecsMeshes>()->segments) {
                if(!seg.mesh) continue;
                
                GLuint vao = seg.mesh->mesh.getVao();
                Material* mat = seg.material.get();
                gfxm::mat4 transform = a->get<ecsWorldTransform>()->transform;
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
                            bone_transforms.emplace_back(t->transform);
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


#include "../common/util/block_vector.hpp"


#include "../common/ecs/systems/scene_graph.hpp"
#include "../common/ecs/systems/animation_sys.hpp"
#include "../common/ecs/attribs/sub_scene_animator.hpp"


#include "../common/ecs/util/assimp_to_ecs_world.hpp"

#include "../common/input_listener.hpp"

#include "../common/render_test.hpp"

#include "../common/kt_cmd.hpp"

class DocEcsWorld : public EditorDocumentTyped<EcsWorld>, public InputListenerWrap {
    ecsWorld root_world;
    std::vector<ecsWorld*> subscene_stack;
    ecsWorld* cur_world;

    entity_id selected_ent = 0;
    ecsRenderSystem* renderSys;
    ecsysSceneGraph* sceneGraphSys;

    GuiViewport gvp;

public:
    DocEcsWorld() {
        //foo_render();

        regEcsAttrib<ecsName>("Name");
        regEcsAttrib<ecsSubScene>("SubScene");
        regEcsAttrib<ecsTagSubSceneRender>("TagSubSceneRender");
        regEcsAttrib<ecsTRS>("TRS");
        regEcsAttrib<ecsTranslation>("Translation");
        regEcsAttrib<ecsRotation>("Rotation");
        regEcsAttrib<ecsScale>("Scale");
        regEcsAttrib<ecsWorldTransform>("WorldTransform");
        regEcsAttrib<ecsParentTransform>("ParentTransform");
        regEcsAttrib<ecsTransform>("Transform");
        regEcsAttrib<ecsTransformTree>("TransformTree");
        regEcsAttrib<ecsVelocity>("Velocity");
        regEcsAttrib<ecsCollisionShape>("CollisionShape", "Collision");
        regEcsAttrib<ecsMass>("Mass", "Physics");
        regEcsAttrib<ecsMeshes>("Meshes", "Rendering");
        regEcsAttrib<ecsSubSceneAnimator>("SubSceneAnimator");

        sceneGraphSys = new ecsysSceneGraph();

        root_world.getSystem<ecsysAnimation>();
        sceneGraphSys = root_world.getSystem<ecsysSceneGraph>();
        root_world.getSystem<ecsDynamicsSys>()->setDebugDraw(&gvp.getDebugDraw());
        renderSys = root_world.getSystem<ecsRenderSystem>();

        auto ent = root_world.createEntity();
        ent.getAttrib<ecsVelocity>();

        gvp.camMode(GuiViewport::CAM_PAN);
        bindActionPress("ALT", [this](){ 
            gvp.camMode(GuiViewport::CAM_ORBIT); 
        });
        bindActionRelease("ALT", [this](){ gvp.camMode(GuiViewport::CAM_PAN); });
    }

    void onGui(Editor* ed, float dt) override {        
        if(subscene_stack.size() > 0) {
            if(ImGui::SmallButton("root")) {
                subscene_stack.clear();
                selected_ent = 0;
            }
            for(size_t i = 0; i < subscene_stack.size(); ++i) {
                ImGui::SameLine();
                ImGui::Text(">");
                ImGui::SameLine();
                if(ImGui::SmallButton( MKSTR(subscene_stack[i]).c_str() )) {
                    subscene_stack.resize(i + 1);
                    selected_ent = 0;
                }
            }

            
        }

        if(!subscene_stack.empty()) {
            cur_world = subscene_stack.back();
        } else {
            cur_world = &root_world;
        }
        
        root_world.update();

        /*
        std::map<uint64_t, size_t> test_map;
        for(int i = 0; i < 100000; ++i) {
            test_map[i] = rand() % 100;
        }
        timer tmr;
        tmr.start();
        size_t value;
        for(auto i = 0; i < 1000; ++i) {
            auto it = test_map.find(rand() % 64);
            value = it != test_map.end() ? it->second : 0;
        }
        LOG("lookup time: " << tmr.stop() << ", value: " << value);
        */

        auto renderSys = cur_world->getSystem<ecsRenderSystem>();

        DrawList dl;
        renderSys->fillDrawList(dl);
        DrawList dl_silhouette;
        renderSys->fillDrawList(dl, selected_ent);

        ImGui::BeginChild(ImGui::GetID("Toolbar0"), ImVec2(0, 32));
        ImGui::Button("Btn0", ImVec2(32, 32));
        ImGui::SameLine();
        ImGui::Button("Btn1", ImVec2(32, 32));
        ImGui::SameLine();
        ImGui::Button("Btn2", ImVec2(32, 32));
        ImGui::SameLine();
        ImGui::Button("Btn3", ImVec2(32, 32));
        ImGui::SameLine();
        ImGui::Button("Btn4", ImVec2(32, 32));
        ImGui::EndChild();

        // === RENDERING ===

        // TODO:
        // 1. Draw objects
        // 2. Debug draw
        // 3. Draw selected marking
        // 4. Draw pick buffer



        // =================
        gvp.draw(dl);
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_RESOURCE")) {
                ResourceNode* node = *(ResourceNode**)payload->Data;
                LOG("Payload received: " << node->getFullName());
                if(has_suffix(node->getName(), ".fbx")) {
                    auto sceneGraphSys = cur_world->getSystem<ecsysSceneGraph>();
                    entity_id ent = sceneGraphSys->createNode();
                    ecsSubScene sub_scene(std::shared_ptr<ecsWorld>(new ecsWorld()));
                    cur_world->setAttrib(ent, sub_scene);
                    
                    ecsysSceneGraph* sceneGraph = sub_scene.getWorld()->getSystem<ecsysSceneGraph>();
                    cur_world->createAttrib<ecsTagSubSceneRender>(ent);
                    cur_world->createAttrib<ecsWorldTransform>(ent);

                    assimpImportEcsScene(sceneGraph, node->getFullName().c_str());
                }
            }
            ImGui::EndDragDropTarget();
        }
    }
    void onGuiToolbox(Editor* ed) override {
        if(!subscene_stack.empty()) {
            cur_world = subscene_stack.back();
        } else {
            cur_world = &root_world;
        }

        if(ImGui::SmallButton(ICON_MDI_PLUS)) {
            selected_ent = cur_world->createEntity().getId();
        }
        ImGui::SameLine();
        if(ImGui::SmallButton(ICON_MDI_MINUS)) {
            cur_world->removeEntity(selected_ent);
        }
        ImGui::PushItemWidth(-1);
        if(ImGui::ListBoxHeader("###OBJECT_LIST", ImVec2(0, 0))) {
            auto sceneGraphSys = cur_world->getSystem<ecsysSceneGraph>();
            sceneGraphSys->onGui(&selected_ent);
                
            ImGui::ListBoxFooter();
        }
        ImGui::PopItemWidth();

        // TODO: Check selected entity validity
        ImGui::Text(MKSTR("UID: " << selected_ent).c_str());

        ImGui::PushItemWidth(-1);
        if(ImGui::BeginCombo("###ADD_ATTRIBUTE", "Add attribute...")) {
            for(auto& it : getEcsAttribTypeLib().getTable()) {
                if(it.first == "") {
                    for(auto& attr_id : it.second) {
                        auto inf = getEcsAttribTypeLib().get_info(attr_id);
                        if(ImGui::Selectable(inf->name.c_str())) {
                            cur_world->createAttrib(selected_ent, attr_id);
                        }
                    }
                } else {
                    bool open = ImGui::TreeNode(it.first.c_str());
                    if(open) {
                        for(auto& attr_id : it.second) {
                            auto inf = getEcsAttribTypeLib().get_info(attr_id);
                            if(ImGui::Selectable(inf->name.c_str())) {
                                cur_world->createAttrib(selected_ent, attr_id);
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
            auto attrib = cur_world->getAttribPtr(selected_ent, i);
            if(!attrib) {
                continue;
            }
            auto inf = getEcsAttribTypeLib().get_info(attrib->get_id());
            const std::string& name = inf->name;
            bool exists = true;
            if(ImGui::CollapsingHeader(MKSTR( name ).c_str(), &exists, ImGuiTreeNodeFlags_DefaultOpen)) {
                attrib->onGui(cur_world, selected_ent);
            }
            if(!exists) {
                cur_world->removeAttrib(selected_ent, i);
            }
        }
    }

    void onFocus() override {
        kt_cmd_set_callback("ecs_world_subdoc", std::bind(&DocEcsWorld::onCmdSubdoc, this, std::placeholders::_1, std::placeholders::_2));
    }
    void onUnfocus() override {
        kt_cmd_clear_callback("ecs_world_subdoc");
    }

    void onCmdSubdoc(int argc, const char* argv[]) {
        if(argc < 2) {
            LOG_WARN("onCmdSubdoc: Not enough arguments");
            return;
        }

        // TODO: Handle exceptions
        uint64_t val = std::stoull(argv[1]);
        ecsWorld* ptr = (ecsWorld*)val;

        if(!ptr) {
            return;
        }

        subscene_stack.push_back(ptr);
        selected_ent = 0;
    }

};
STATIC_RUN(DocEcsWorld) {
    regEditorDocument<DocEcsWorld>({ "ecsw" });
}

#endif
