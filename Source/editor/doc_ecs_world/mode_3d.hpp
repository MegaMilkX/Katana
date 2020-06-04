#ifndef DOC_ECS_WORLD_3D_MODE_HPP
#define DOC_ECS_WORLD_3D_MODE_HPP

#include "mode.hpp"
#include "../editor_async_task/editor_async_task.hpp"
#include <memory>


void draw2d(const DrawList2d& dl, float screenW, float screenH);
void drawText(Font* fnt, const std::string& str, float screenW, float screenH, float x, float y);

class DocEcsWorldMode3d : public DocEcsWorldMode {
    std::unique_ptr<edTaskEcsWorldCalcLightmaps>            lightmap_task;
    std::set<std::unique_ptr<edTaskEcsWorldModelDragNDrop>> model_dnd_tasks;

    CursorData                        cursor_data;
    std::vector<SpawnTweakStage>      spawn_tweak_stage_stack;
    int                               cur_tweak_stage_id = 0;

    gl::FrameBuffer fb_outline;
    gl::FrameBuffer fb_blur;
    gl::FrameBuffer fb_silhouette;
    gl::FrameBuffer fb_pick;

    Font font;

public:
    DocEcsWorldMode3d() {
        fb_silhouette.addBuffer(0, GL_RED, GL_UNSIGNED_BYTE);
        fb_outline.addBuffer(0, GL_RED, GL_UNSIGNED_BYTE);
        fb_blur.addBuffer(0, GL_RED, GL_UNSIGNED_BYTE);
        fb_pick.addBuffer(0, GL_RGB, GL_UNSIGNED_INT);

        font.load("OpenSans-Regular.ttf");
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
        for(auto& t : model_dnd_tasks) {
            if(t->isDone()) {
                auto sceneGraphSys = t->target_world->getSystem<ecsysSceneGraph>();
                entity_id ent = sceneGraphSys->createNode();
                t->target_world->setAttrib(ent, t->subscene);            
                
                t->target_world->createAttrib<ecsTagSubSceneRender>(ent);
                t->target_world->createAttrib<ecsWorldTransform>(ent);

                t->target_world->createAttrib<ecsName>(ent);
                t->target_world->getAttrib<ecsName>(ent)->name = t->entity_name;
                state.selected_ent = ent;

                model_dnd_tasks.erase(t);
                break;
            }
        }

        auto renderSys = state.world->getSystem<ecsRenderSystem>();
        DrawList dl_silhouette;
        renderSys->fillDrawList(dl_silhouette, state.selected_ent);

        if(state.gvp.begin()) {
            state.gvp.getRenderer()->draw(state.gvp.getViewport(), state.gvp.getProjection(), state.gvp.getView(), state.dl);
            fb_silhouette.reinitBuffers(state.gvp.getViewport()->getWidth(), state.gvp.getViewport()->getHeight());
            fb_outline.reinitBuffers(state.gvp.getViewport()->getWidth(), state.gvp.getViewport()->getHeight());
            fb_blur.reinitBuffers(state.gvp.getViewport()->getWidth(), state.gvp.getViewport()->getHeight());
            fb_pick.reinitBuffers(state.gvp.getViewport()->getWidth(), state.gvp.getViewport()->getHeight());
        }
        state.gvp.end();

        auto render2d = state.world->getSystem<ecsRenderGui>();
        auto dl2d = render2d->getDrawList();
        state.gvp.getViewport()->getFinalBuffer()->bind();
        draw2d(dl2d, state.gvp.getSize().x, state.gvp.getSize().y);

        state.gvp.getRenderer()->drawSilhouettes(&fb_silhouette, state.gvp.getProjection(), state.gvp.getView(), dl_silhouette);
        outline(&fb_outline, fb_silhouette.getTextureId(0));
        cutout(&fb_blur, fb_outline.getTextureId(0), fb_silhouette.getTextureId(0));/*
        blur(&fb_outline, fb_silhouette.getTextureId(0), gfxm::vec2(1, 0));
        blur(&fb_blur, fb_outline.getTextureId(0), gfxm::vec2(0, 1));
        blur(&fb_outline, fb_blur.getTextureId(0), gfxm::vec2(1, 0));
        blur(&fb_blur, fb_outline.getTextureId(0), gfxm::vec2(0, 1));
        cutout(&fb_outline, fb_blur.getTextureId(0), fb_silhouette.getTextureId(0));*/
        overlay(state.gvp.getViewport()->getFinalBuffer(), fb_blur.getTextureId(0));
        state.gvp.getViewport()->getFinalBuffer()->bind();
        static float counter = 0;
        drawText(
            &font, 
            MKSTR("General text quality test: " << (int)counter << "\n" << "This is a new line" << "\nHello World!").c_str(), 
            state.gvp.getSize().x, state.gvp.getSize().y,
            (int)(sinf(counter) * 100.0f) + 100.0f, (int)(cosf(counter) * 100.0f) + 200.0f);
        counter += (1.0f/60.0f);

        if (!state.world->getEntities().empty()) {
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
                if(ImGuizmo::IsUsing()) {
                    if(tr) {
                        tr->translate(dm[3]);
                    }
                }
            }
        }
        auto mpos = state.gvp.getMousePos();
        if(state.gvp.isMouseClicked(0) && !ImGuizmo::IsUsing()) {
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
                    entity_id ent = (entity_id)state.dl.skins[pix - state.dl.solids.size()].object_ptr;
                    state.selected_ent = ent;
                    LOG("PICKED_SKIN: " << pix - state.dl.solids.size());
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
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_RESOURCE")) {
                ResourceNode* node = *(ResourceNode**)payload->Data;
                LOG("Payload received: " << node->getFullName());
                if(has_suffix(node->getName(), ".entity")) {
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
                    state.backupState();
                    edTaskEcsWorldModelDragNDrop* task = new edTaskEcsWorldModelDragNDrop(
                        MKSTR("Importing " << node->getFullName()).c_str(),
                        node->getFullName().c_str(),
                        node->getName().c_str(),
                        state.world
                    );
                    model_dnd_tasks.insert(std::unique_ptr<edTaskEcsWorldModelDragNDrop>(task));
                    edTaskPost(task);
                    /*
                    auto sceneGraphSys = state.world->getSystem<ecsysSceneGraph>();
                    entity_id ent = sceneGraphSys->createNode();
                    ecsSubScene sub_scene(std::shared_ptr<ecsWorld>(new ecsWorld()));
                    state.world->setAttrib(ent, sub_scene);
                    
                    ecsysSceneGraph* sceneGraph = sub_scene.getWorld()->getSystem<ecsysSceneGraph>();
                    state.world->createAttrib<ecsTagSubSceneRender>(ent);
                    state.world->createAttrib<ecsWorldTransform>(ent);

                    assimpImportEcsScene(sceneGraph, node->getFullName().c_str());

                    state.world->createAttrib<ecsName>(ent);
                    state.world->getAttrib<ecsName>(ent)->name = node->getName();
                    selected_ent = ent;

                    spawn_tweak_stage_stack.push_back(SpawnTweakStage(
                        ecsEntityHandle(state.world, selected_ent), cursor_data, [](ecsEntityHandle e, const SpawnTweakData& c){
                            e.getAttrib<ecsTranslation>()->setPosition(c.cursor.pos + c.cursor.normal * 0.5f);
                            return c.cursor_start;
                        }
                    ));*/
                }
            }
            ImGui::EndDragDropTarget();
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
            h.getAttrib<ecsWorldTransform>();
            h.getAttrib<ecsTranslation>();
            h.getAttrib<ecsRotation>();
            h.getAttrib<ecsScale>();
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
            std::map<entity_id, std::vector<entity_id>> child_map;
            std::vector<entity_id> top_level_entities;
            auto& entities = state.world->getEntities();
            for(auto e : entities) {
                ecsEntityHandle hdl(state.world, e);
                ecsParentTransform* pt = hdl.findAttrib<ecsParentTransform>();
                if(pt && pt->parent_entity >= 0) {
                    child_map[pt->parent_entity].emplace_back(hdl.getId());
                    continue;
                }
                top_level_entities.emplace_back(hdl.getId());
            }
            
            if(search_query.empty()) {
                imguiEntityList(state.world, top_level_entities, child_map, state.selected_ent);
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
                imguiEntityList(state.world, found_entities, child_map, state.selected_ent);
            }
            ImGui::ListBoxFooter();
        }
        ImGui::PopItemWidth();

        imguiEntityAttributeList(state);
    }
};


#endif
