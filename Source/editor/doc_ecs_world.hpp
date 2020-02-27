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

#include "../common/ecs/systems/dynamics.hpp"

#include "../common/ecs/systems/render.hpp"



#include "../common/util/block_vector.hpp"


#include "../common/ecs/systems/scene_graph.hpp"
#include "../common/ecs/systems/animation_sys.hpp"
#include "../common/ecs/attribs/sub_scene_animator.hpp"


#include "../common/ecs/util/assimp_to_ecs_world.hpp"

#include "../common/input_listener.hpp"

#include "../common/render_test.hpp"

#include "../common/kt_cmd.hpp"

#include "../common/util/filesystem.hpp"

class DocEcsWorld : public EditorDocumentTyped<ecsWorld>, public InputListenerWrap {
    std::vector<ecsWorld*> subscene_stack;
    ecsWorld* cur_world;

    entity_id selected_ent = 0;
    ecsRenderSystem* renderSys;
    ecsysSceneGraph* sceneGraphSys;

    GuiViewport gvp;
    gl::FrameBuffer fb_outline;
    gl::FrameBuffer fb_blur;
    gl::FrameBuffer fb_silhouette;
    gl::FrameBuffer fb_pick;

public:
    DocEcsWorld() {
        //foo_render();

        setIconCode(ICON_MDI_ALPHA_W_BOX);

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

        //sceneGraphSys = new ecsysSceneGraph();

        _resource->getSystem<ecsysAnimation>();
        //sceneGraphSys = root_world.getSystem<ecsysSceneGraph>();
        _resource->getSystem<ecsDynamicsSys>()->setDebugDraw(&gvp.getDebugDraw());
        renderSys = _resource->getSystem<ecsRenderSystem>();

        auto ent = _resource->createEntity();
        ent.getAttrib<ecsVelocity>();

        gvp.camMode(GuiViewport::CAM_PAN);
        fb_silhouette.pushBuffer(GL_RED, GL_UNSIGNED_BYTE);
        fb_outline.pushBuffer(GL_RED, GL_UNSIGNED_BYTE);
        fb_blur.pushBuffer(GL_RED, GL_UNSIGNED_BYTE);
        fb_pick.pushBuffer(GL_RGB, GL_UNSIGNED_INT);

        bindActionPress("ALT", [this](){ 
            gvp.camMode(GuiViewport::CAM_ORBIT); 
        });
        bindActionRelease("ALT", [this](){ gvp.camMode(GuiViewport::CAM_PAN); });
    }

    void onResourceSet() override {
        _resource->getSystem<ecsysAnimation>();
        _resource->getSystem<ecsDynamicsSys>()->setDebugDraw(&gvp.getDebugDraw());
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
            cur_world = _resource.get();
        }
        
        _resource->update();

        auto renderSys = cur_world->getSystem<ecsRenderSystem>();
        DrawList dl;
        renderSys->fillDrawList(dl);
        DrawList dl_silhouette;
        renderSys->fillDrawList(dl_silhouette, selected_ent);

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
        //gvp.draw(dl);
        if(gvp.begin()) {
            gvp.getRenderer()->draw(gvp.getViewport(), gvp.getProjection(), gvp.getView(), dl);
            fb_silhouette.reinitBuffers(gvp.getViewport()->getWidth(), gvp.getViewport()->getHeight());
            fb_outline.reinitBuffers(gvp.getViewport()->getWidth(), gvp.getViewport()->getHeight());
            fb_blur.reinitBuffers(gvp.getViewport()->getWidth(), gvp.getViewport()->getHeight());
            fb_pick.reinitBuffers(gvp.getViewport()->getWidth(), gvp.getViewport()->getHeight());

            gvp.getRenderer()->drawSilhouettes(&fb_silhouette, dl_silhouette);
            blur(&fb_outline, fb_silhouette.getTextureId(0), gfxm::vec2(1, 0));
            blur(&fb_blur, fb_outline.getTextureId(0), gfxm::vec2(0, 1));
            blur(&fb_outline, fb_blur.getTextureId(0), gfxm::vec2(1, 0));
            blur(&fb_blur, fb_outline.getTextureId(0), gfxm::vec2(0, 1));
            cutout(&fb_outline, fb_blur.getTextureId(0), fb_silhouette.getTextureId(0));
            overlay(gvp.getViewport()->getFinalBuffer(), fb_outline.getTextureId(0));

            if(gvp.isMouseClicked(0)) {
                auto mpos = gvp.getMousePos();
                gvp.getRenderer()->drawPickPuffer(&fb_pick, dl);
                
                uint32_t pix = 0;
                uint32_t err_pix = 0xFFFFFF;
                glReadPixels(mpos.x, mpos.y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, &pix);
                if(pix == err_pix) {
                    selected_ent = 0;
                } else if(pix < dl.solids.size()) {
                    entity_id ent = (entity_id)dl.solids[pix].object_ptr;
                    selected_ent = ent;
                    LOG("PICKED_SOLID: " << pix);
                } else {
                    entity_id ent = (entity_id)dl.skins[pix - dl.solids.size()].object_ptr;
                    selected_ent = ent;
                    LOG("PICKED_SKIN: " << pix - dl.solids.size());
                }
            }
        }
        gvp.end();
        if(ImGui::BeginPopupContextWindow()) {
            ImGui::MenuItem("Create entity...");
            ImGui::MenuItem("Instantiate");
            ImGui::EndPopup();
        }
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

                    cur_world->createAttrib<ecsName>(ent);
                    cur_world->getAttrib<ecsName>(ent)->name = node->getName();
                    selected_ent = ent;
                }
            }
            ImGui::EndDragDropTarget();
        }
    }
    void onGuiToolbox(Editor* ed) override {
        if(!subscene_stack.empty()) {
            cur_world = subscene_stack.back();
        } else {
            cur_world = _resource.get();
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

        for(uint8_t i = 0; i < get_last_attrib_id() + 1; ++i) {
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
