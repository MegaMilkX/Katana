#ifndef DOC_ECS_WORLD_3D_MODE_HPP
#define DOC_ECS_WORLD_3D_MODE_HPP

#include "mode.hpp"
#include "../editor_async_task/editor_async_task.hpp"
#include <memory>


void draw2d(const DrawList2d& dl, float screenW, float screenH);

enum GIZMO_RECT_CONTROL_POINT {
    GIZMO_RECT_NONE     = 0x000,
    GIZMO_RECT_NORTH    = 0x001,
    GIZMO_RECT_SOUTH    = 0x002,
    GIZMO_RECT_WEST     = 0x004,
    GIZMO_RECT_EAST     = 0x008,
    GIZMO_RECT_NW       = 0x010,
    GIZMO_RECT_NE       = 0x020,
    GIZMO_RECT_SE       = 0x040,
    GIZMO_RECT_SW       = 0x080,
    GIZMO_RECT_ORIGIN   = 0x100,
    GIZMO_RECT_BOX      = 0x200,
    GIZMO_RECT_ROTATE   = 0x400,
    
    GIZMO_RECT_CP_ALL   = 0xFFF
};

void gizmoRect2dViewport(float left, float right, float bottom, float top);
bool gizmoRect2d(gfxm::mat4& t, const gfxm::mat4& local, gfxm::vec2& lcl_pos_offs, float& rotation, gfxm::vec2& origin, gfxm::rect& rect, const gfxm::vec2& mouse, bool button_pressed, int cp_flags);
void gizmoRect2dDraw(float screenW, float screenH);

class DocEcsWorldMode3d : public DocEcsWorldMode {
    std::unique_ptr<edTaskEcsWorldCalcLightmaps>            lightmap_task;

    CursorData                        cursor_data;
    std::vector<SpawnTweakStage>      spawn_tweak_stage_stack;
    int                               cur_tweak_stage_id = 0;

    gl::FrameBuffer fb_outline;
    gl::FrameBuffer fb_blur;
    gl::FrameBuffer fb_silhouette;
    gl::FrameBuffer fb_pick;

    std::shared_ptr<Font> font;

public:
    DocEcsWorldMode3d() {
        fb_silhouette.addBuffer(0, GL_RED, GL_UNSIGNED_BYTE);
        fb_outline.addBuffer(0, GL_RED, GL_UNSIGNED_BYTE);
        fb_blur.addBuffer(0, GL_RED, GL_UNSIGNED_BYTE);
        fb_pick.addBuffer(0, GL_RGB, GL_UNSIGNED_INT);

        //font = getResource<Font>("fonts/OpenSans-Regular.ttf");
    }

    const char* getName() const override { return "3D"; }

    void onMenuBar(DocEcsWorldState& state) override {
        if(ImGui::BeginMenu("Create")) {
            if (ImGui::MenuItem("Omni Light")) {
                ecsEntityHandle hdl = state.world->createEntity();
                hdl.getAttrib<ecsTranslation>();
                hdl.getAttrib<ecsWorldTransform>();
                hdl.getAttrib<ecsLightOmni>();
                state.selected_ent = hdl.getId();
            }
            ImGui::EndMenu();
        }

        if(ImGui::MenuItem("Bake Lightmaps")) {
            lightmap_task = std::make_unique<edTaskEcsWorldCalcLightmaps>("Calculating lightmaps");
            lightmap_task->dl = state.dl;
            lightmap_task->gvp = &state.gvp;
            lightmap_task->renderSys = state.world->getSystem<ecsRenderSystem>();
            edTaskPost(lightmap_task.get());
        }

        if(ImGui::MenuItem("Reload Shaders")) {
            shaderLoader().reloadAll();
        }
    }

    void onMainWindow(DocEcsWorldState& state) override {
        // Check if any async tasks finished
        for(auto& t : state.model_dnd_tasks) {
            if(t->isDone()) {
                if(t->as_subscene) {
                    auto ent = t->target_world->createEntity();
                    t->target_world->setAttrib(ent.getId(), t->subscene);            
                    
                    ent.getAttrib<ecsTagSubSceneRender>();
                    ent.getAttrib<ecsWorldTransform>();

                    ent.getAttrib<ecsName>()->name = t->entity_name;
                    state.selected_ent = ent.getId();
                }
                state.model_dnd_tasks.erase(t);
                break;
            }
        }

        if(state.gvp.begin()) {
            state.gvp.getRenderer()->draw(state.gvp.getViewport(), state.gvp.getProjection(), state.gvp.getView(), state.dl);
            fb_silhouette.reinitBuffers(state.gvp.getViewport()->getWidth(), state.gvp.getViewport()->getHeight());
            fb_outline.reinitBuffers(state.gvp.getViewport()->getWidth(), state.gvp.getViewport()->getHeight());
            fb_blur.reinitBuffers(state.gvp.getViewport()->getWidth(), state.gvp.getViewport()->getHeight());
            fb_pick.reinitBuffers(state.gvp.getViewport()->getWidth(), state.gvp.getViewport()->getHeight());
        }
        state.gvp.end();

        auto render2d = state.world->getSystem<ecsRenderGui>();
        auto dl2d = render2d->makeDrawList(state.gvp.getSize().x, state.gvp.getSize().y, state.selected_ent);
        state.gvp.getViewport()->getFinalBuffer()->bind();
        draw2d(dl2d, state.gvp.getSize().x, state.gvp.getSize().y);

        state.gvp.getRenderer()->drawSilhouettes(&fb_silhouette, state.gvp.getProjection(), state.gvp.getView(), state.dl);
        outline(&fb_outline, fb_silhouette.getTextureId(0));
        cutout(&fb_blur, fb_outline.getTextureId(0), fb_silhouette.getTextureId(0));/*
        blur(&fb_outline, fb_silhouette.getTextureId(0), gfxm::vec2(1, 0));
        blur(&fb_blur, fb_outline.getTextureId(0), gfxm::vec2(0, 1));
        blur(&fb_outline, fb_blur.getTextureId(0), gfxm::vec2(1, 0));
        blur(&fb_blur, fb_outline.getTextureId(0), gfxm::vec2(0, 1));
        cutout(&fb_outline, fb_blur.getTextureId(0), fb_silhouette.getTextureId(0));*/
        overlay(state.gvp.getViewport()->getFinalBuffer(), fb_blur.getTextureId(0));
        /*
        state.gvp.getViewport()->getFinalBuffer()->bind();
        static float counter = 0;
        drawText(
            font.get(), 
            MKSTR("General text quality test: " << (int)counter << "\n" << "This is a new line" << "\nHello World!").c_str(), 
            state.gvp.getSize().x, state.gvp.getSize().y,
            (int)(sinf(counter) * 100.0f) + 100.0f, (int)(cosf(counter) * 100.0f) + 200.0f);
        counter += (1.0f/60.0f);*/

        bool using_gizmo = false;

        float vp_w = state.gvp.getSize().x;
        float vp_h = state.gvp.getSize().y;
        ecsEntityHandle hdl(state.world, state.selected_ent);
        if(hdl.isValid()){
            auto elem = hdl.findAttrib<ecsGuiElement>();
            auto gui_text = hdl.findAttrib<ecsGuiText>();
            auto gui_img  = hdl.findAttrib<ecsGuiImage>();

            if(elem) {
                gizmoRect2dViewport(0, vp_w, 0, vp_h);

                gfxm::rect rect = elem->getBoundingRect();
                auto& io = ImGui::GetIO();
                auto impos = state.gvp.getMousePos();
                gfxm::vec2 mpos(impos.x, impos.y);
                gfxm::rect rect_delta;
                int giz_flags = GIZMO_RECT_CP_ALL;

                gfxm::mat4 transform = elem->getDrawTransform();
                gfxm::vec2 pos_offs;
                float      rotation = elem->getRotation();
                gfxm::vec2 origin = elem->getOrigin();
                using_gizmo = gizmoRect2d(transform, elem->getLocalDrawTransform(), pos_offs, rotation, origin, rect, mpos, io.MouseDown[0], giz_flags);
                if(using_gizmo) {
                    elem->translate(pos_offs);
                    elem->setRotation(rotation);
                    elem->setOrigin(origin);
                    if(gui_img) {
                        gui_img->setSize(rect.max.x - rect.min.x, rect.max.y - rect.min.y);
                    }
                }

                state.gvp.getViewport()->getFinalBuffer()->bind();
                gizmoRect2dDraw(state.gvp.getSize().x, state.gvp.getSize().y);
            }
        }
        if (!state.world->getEntities().empty() && !using_gizmo) {
            auto tr = state.world->findAttrib<ecsTranslation>(state.selected_ent);
            auto rot = state.world->findAttrib<ecsRotation>(state.selected_ent);
            auto wt = state.world->findAttrib<ecsWorldTransform>(state.selected_ent);
            gfxm::mat4 m(1.0f);
            if(tr) {
                m = gfxm::translate(m, tr->getPosition());
            }
            if(rot) {
                m = m * gfxm::to_mat4(rot->getRotation());
            }
            if(tr) {
                gfxm::mat4 dm(1.0f);
                float snap = 0.1f;
                ImGuizmo::Manipulate(
                    (float*)&state.gvp.getView(), 
                    (float*)&state.gvp.getProjection(),
                    ImGuizmo::TRANSLATE, ImGuizmo::MODE::LOCAL,
                    (float*)&m, (float*)&dm, 0 /* snap */
                );
                using_gizmo = ImGuizmo::IsUsing();
                if(ImGuizmo::IsUsing()) {
                    if(tr) {
                        tr->translate(dm[3]);
                    }
                }
            }
        }
        

        auto mpos = state.gvp.getMousePos();
        if(state.gvp.isMouseClicked(0) && !using_gizmo) {
            if(cur_tweak_stage_id < spawn_tweak_stage_stack.size()) {
                ++cur_tweak_stage_id;
                if(cur_tweak_stage_id == spawn_tweak_stage_stack.size()) {
                    cur_tweak_stage_id = 0;
                    spawn_tweak_stage_stack.clear();
                }
            } else {
                state.gvp.getRenderer()->drawPickPuffer(&fb_pick, state.gvp.getProjection(), state.gvp.getView(), state.dl);
                
                uint32_t pix = 0;
                uint32_t err_pix = 0xFFFFFF;
                glReadPixels(mpos.x, mpos.y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, &pix);
                if(pix == err_pix) {
                    state.selected_ent = 0;
                } else if(pix < state.dl.solids.size()) {
                    entity_id ent = (entity_id)state.dl.solids[pix].object_ptr;
                    state.selected_ent = ent;
                    LOG("PICKED_SOLID: " << pix);
                } else {
                    assert(false);
                }
            }
        }


        cursor_data.normal = state.gvp.getCursor3dNormal();
        cursor_data.pos = state.gvp.getCursor3d();
        cursor_data.xy_plane_pos = state.gvp.getCursorXYPlane();
        if(cur_tweak_stage_id < spawn_tweak_stage_stack.size()) {
            // TODO:
            CursorData cd = spawn_tweak_stage_stack[cur_tweak_stage_id].update(cursor_data);
            if(cur_tweak_stage_id + 1 < spawn_tweak_stage_stack.size()) {
                spawn_tweak_stage_stack[cur_tweak_stage_id + 1].setBaseCursor(cd);
            }
        }

        if(ImGui::BeginPopupContextWindow()) {
            ImGui::MenuItem("Create entity...");
            ImGui::MenuItem("Instantiate");
            ImGui::EndPopup();
        }
        ImGuiID fbxPopupId = ImGui::GetID("fbxPopupId");
        static ResourceNode* dragged_node = 0;
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_RESOURCE")) {
                ResourceNode* node = *(ResourceNode**)payload->Data;
                LOG("Payload received: " << node->getFullName());
                if(has_suffix(node->getName(), ".ecsw")) {
                    state.backupState();
                    std::string fname = node->getFullName();
                    auto hdl = state.world->mergeWorld(fname.c_str());
                    if(hdl.isValid()) {
                        state.selected_ent = hdl.getId();
                    }
                } else if(has_suffix(node->getName(), ".entity")) {
                    state.backupState();
                    std::string fname = node->getFullName();
                    file_stream f(get_module_dir() + "/" + platformGetConfig().data_dir + "/" + fname, file_stream::F_IN);
                    if(f.is_open()) {
                        auto ehdl = state.world->createEntityFromTemplate(node->getFullName().c_str());
                        if(ehdl.isValid()) {
                            state.selected_ent = ehdl.getId();
                        }
                    }
                } else if(has_suffix(node->getName(), ".fbx")) {
                    dragged_node = node;
                    ImGui::OpenPopupEx(fbxPopupId);
                } else if(has_suffix(node->getName(), ".png") || has_suffix(node->getName(), ".jpg") || has_suffix(node->getName(), ".jpeg") || has_suffix(node->getName(), ".tga")) {
                    state.backupState();
                    auto hdl = state.world->createEntity();
                    hdl.getAttrib<ecsGuiElement>();
                    auto res = node->getResource<Texture2D>();
                    if(res) {
                        hdl.getAttrib<ecsGuiImage>()->setImage(res);
                        hdl.getAttrib<ecsGuiImage>()->setSize(res->Width(), res->Height());
                    }
                    state.selected_ent = hdl.getId();
                }
            }
            ImGui::EndDragDropTarget();
        }
        if(ImGui::BeginPopupEx(fbxPopupId, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_Modal | ImGuiWindowFlags_NoResize)) {
            if(ImGui::Button("As Model")) {
                auto hdl = state.world->createEntity();
                auto ecsmdl = hdl.getAttrib<ecsModel>();
                std::shared_ptr<Model_> mdl(new Model_);
                assimpImportEcsModel(mdl.get(), dragged_node->getFullName().c_str());
                ecsmdl->model = mdl;
                state.selected_ent = hdl.getId();
                hdl.getAttrib<ecsName>()->name = dragged_node->getName();
                hdl.getAttrib<ecsWorldTransform>();
                ImGui::CloseCurrentPopup();
            }
            if(ImGui::Button("Merge")) {
                entity_id e = assimpImportEcsScene(state.world, dragged_node->getFullName().c_str());
                state.selected_ent = e;
                ImGui::CloseCurrentPopup();
            }
            if(ImGui::Button("As SubScene")) {
                state.backupState();
                edTaskEcsWorldModelDragNDrop* task = new edTaskEcsWorldModelDragNDrop(
                    MKSTR("Importing " << dragged_node->getFullName()).c_str(),
                    dragged_node->getFullName().c_str(),
                    dragged_node->getName().c_str(),
                    state.world
                );
                state.model_dnd_tasks.insert(std::unique_ptr<edTaskEcsWorldModelDragNDrop>(task));
                edTaskPost(task);
                ImGui::CloseCurrentPopup();
            }
            if(ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }        
    }

    void onToolbox(DocEcsWorldState& state) override {
        /*
        for(int i = 0; i < state.world->systemCount(); ++i) {
            auto sys = state.world->getSystem(i);
            bool exists = true;
            if(ImGui::CollapsingHeader(MKSTR( sys ).c_str(), &exists, ImGuiTreeNodeFlags_DefaultOpen)) {
                
            }
        }*/

        if(ImGui::SmallButton(ICON_MDI_PLUS " Create")) {
            state.backupState();
            state.selected_ent = state.world->createEntity().getId();
            ecsEntityHandle h(state.world, state.selected_ent);
            h.getAttrib<ecsName>()->name = "New object";
            h.getAttrib<ecsTranslation>();
            h.getAttrib<ecsRotation>();
            h.getAttrib<ecsWorldTransform>();
        }
        ImGui::SameLine();
        if(ImGui::SmallButton(ICON_MDI_MINUS " Remove")) {
            state.backupState();
            state.world->removeEntity(state.selected_ent);
        }

        static std::string search_query;
        char buf[256];
        memset(buf, 0, 256);
        memcpy(buf, search_query.data(), search_query.size());
        if(ImGui::InputText(ICON_MDI_SEARCH_WEB, buf, 256)) {
            search_query = buf;
        }
        ImGui::PushItemWidth(-1);
        if(ImGui::ListBoxHeader("###OBJECT_LIST", ImVec2(0, 0))) {
            std::vector<entity_id> top_level_entities;
            auto& entities = state.world->getEntities();
            for(auto e : entities) {
                ecsEntityHandle hdl(state.world, e);
                if(state.world->getParent(e) == NULL_ENTITY) {
                    top_level_entities.emplace_back(hdl.getId());
                }
            }
            
            if(search_query.empty()) {
                imguiEntityList_(state.world, top_level_entities, state.selected_ent);
            } else {
                std::vector<entity_id> found_entities;
                auto& entities = state.world->getEntities();
                for(auto e : entities) {
                    ecsEntityHandle hdl(state.world, e);
                    ecsName* name = hdl.findAttrib<ecsName>();
                    if(!name) {
                        continue;
                    }
                    std::string a = name->name;
                    std::string b = search_query;
                    for(auto& c : a) c = std::toupper(c);
                    for(auto& c : b) c = std::toupper(c);
                    if(strncmp(a.c_str(), b.c_str(), b.size()) == 0) {
                        found_entities.emplace_back(hdl.getId());
                    }
                }
                imguiEntityList_(state.world, found_entities, state.selected_ent);
            }
            ImGui::ListBoxFooter();
        }
        ImGui::PopItemWidth();

        imguiEntityAttributeList(state);
    }
};


#endif
