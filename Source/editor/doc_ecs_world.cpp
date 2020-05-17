#include "doc_ecs_world.hpp"

DocEcsWorld::DocEcsWorld() {
    //foo_render();

    setIconCode(ICON_MDI_ALPHA_W_BOX);

    _resource->getSystem<ecsysAnimation>();
    _resource->getSystem<ecsysSceneGraph>();
    _resource->getSystem<ecsDynamicsSys>()->setDebugDraw(&gvp.getDebugDraw());
    renderSys = _resource->getSystem<ecsRenderSystem>();

    auto ent = _resource->createEntity();
    ent.getAttrib<ecsVelocity>();

    gvp.camMode(GuiViewport::CAM_PAN);
    fb_silhouette.addBuffer(0, GL_RED, GL_UNSIGNED_BYTE);
    fb_outline.addBuffer(0, GL_RED, GL_UNSIGNED_BYTE);
    fb_blur.addBuffer(0, GL_RED, GL_UNSIGNED_BYTE);
    fb_pick.addBuffer(0, GL_RGB, GL_UNSIGNED_INT);

    bindActionPress("ALT", [this](){ 
        gvp.camMode(GuiViewport::CAM_ORBIT); 
    });
    bindActionRelease("ALT", [this](){ gvp.camMode(GuiViewport::CAM_PAN); });
}

void DocEcsWorld::onResourceSet() {
    _resource->getSystem<ecsysAnimation>();
    _resource->getSystem<ecsDynamicsSys>()->setDebugDraw(&gvp.getDebugDraw());
    _resource->getSystem<ecsysSceneGraph>();
}

void DocEcsWorld::onGui(Editor* ed, float dt) {        
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

    ImGuiID createPopupId = ImGui::GetID("CreatePopupMenu");

    ImGui::BeginChild(ImGui::GetID("Toolbar0"), ImVec2(0, 32));
    
    if(ImGui::Button("Create", ImVec2(0, 32))) {
        if(!ImGui::IsPopupOpen(createPopupId)) {
            ImGui::OpenPopupEx(createPopupId);
        }
    }
    ImGui::SameLine();
    if(ImGui::Button("Bake Lightmaps", ImVec2(0, 32))) {
        auto entities = renderSys->get_array<ecsTuple<ecsWorldTransform, ecsMeshes>>();
        std::vector<LightmapMeshData> mesh_data;
        for(auto& e : entities) {
            auto m = e->get<ecsMeshes>();
            for(auto& seg : m->segments) {
                if(!seg.mesh) {
                    continue;
                }
                if(!seg.mesh->mesh.getAttribBuffer(VERTEX_FMT::ENUM_GENERIC::UVLightmap)
                    || !seg.mesh->mesh.getAttribBuffer(VERTEX_FMT::ENUM_GENERIC::Normal)
                    || !seg.mesh->mesh.getAttribBuffer(VERTEX_FMT::ENUM_GENERIC::Position)
                ) {
                    continue;
                }

                mesh_data.resize(mesh_data.size() + 1);
                auto& md = mesh_data.back();

                md.transform = e->get<ecsWorldTransform>()->transform;
                md.tex_width = 256;
                md.tex_height = 256;
                md.position.resize(seg.mesh->vertexCount() * 3);
                seg.mesh->mesh.copyAttribData(VERTEX_FMT::ENUM_GENERIC::Position, md.position.data());
                md.normal.resize(seg.mesh->vertexCount() * 3);
                seg.mesh->mesh.copyAttribData(VERTEX_FMT::ENUM_GENERIC::Normal, md.normal.data());
                md.uv_lightmap.resize(seg.mesh->vertexCount() * 2);
                seg.mesh->mesh.copyAttribData(VERTEX_FMT::ENUM_GENERIC::UVLightmap, md.uv_lightmap.data());
                md.indices.resize(seg.mesh->indexCount());
                seg.mesh->mesh.copyIndexData(md.indices.data());
                md.segment = &seg;
            }
        }

        // TODO: USE SEPARATE GBUFFER WTF
        GenLightmaps(mesh_data, gvp.getRenderer(), gvp.getViewport()->getGBuffer(), dl);

        int i = 0;
        for(auto& e : entities) {
            auto m = e->get<ecsMeshes>();
            for(auto& seg : m->segments) {
                if(!seg.mesh) {
                    continue;
                }
                if(!seg.mesh->mesh.getAttribBuffer(VERTEX_FMT::ENUM_GENERIC::UVLightmap)
                    || !seg.mesh->mesh.getAttribBuffer(VERTEX_FMT::ENUM_GENERIC::Normal)
                    || !seg.mesh->mesh.getAttribBuffer(VERTEX_FMT::ENUM_GENERIC::Position)
                ) {
                    continue;
                }

                seg.lightmap = mesh_data[i].lightmap;
                
                ++i;
            }
        }
    }
    ImGui::SameLine();
    if(ImGui::Button("Reload Shaders", ImVec2(0, 32))) {
        shaderLoader().reloadAll();
    }
    ImGui::SameLine();
    ImGui::Button("Btn1", ImVec2(32, 32));
    ImGui::SameLine();
    ImGui::Button("Btn2", ImVec2(32, 32));
    ImGui::SameLine();
    ImGui::Button("Btn3", ImVec2(32, 32));
    ImGui::SameLine();
    ImGui::Button("Btn4", ImVec2(32, 32));
    ImGui::EndChild();

    {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_Popup | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;
        
        if(ImGui::BeginPopupEx(createPopupId, window_flags)) {
            if (ImGui::MenuItem("Omni Light")) {
                ecsEntityHandle hdl = _resource->createEntity();
                hdl.getAttrib<ecsTranslation>();
                hdl.getAttrib<ecsWorldTransform>();
                hdl.getAttrib<ecsLightOmni>();
                selected_ent = hdl.getId();

                spawn_tweak_stage_stack.push_back(SpawnTweakStage(
                    hdl, cursor_data, [](ecsEntityHandle e, const SpawnTweakData& c){
                        e.getAttrib<ecsTranslation>()->setPosition(c.cursor.pos + c.cursor.normal * 0.5f);
                        CursorData cd = c.cursor_start;
                        cd.pos = e.getAttrib<ecsTranslation>()->getPosition();
                        return cd;
                    }
                ));
                spawn_tweak_stage_stack.push_back(SpawnTweakStage(
                    hdl, cursor_data, [](ecsEntityHandle e, const SpawnTweakData& c){
                        e.getAttrib<ecsLightOmni>()->radius = gfxm::length(c.world_delta);
                        return c.cursor_start;
                    }
                ));
                spawn_tweak_stage_stack.push_back(SpawnTweakStage(
                    hdl, cursor_data, [](ecsEntityHandle e, const SpawnTweakData& c){
                        e.getAttrib<ecsLightOmni>()->intensity = gfxm::length(c.world_delta) * 10.0f;
                        return c.cursor_start;
                    }
                ));
            }
            ImGui::EndPopup();
        }            
    }

    if(gvp.begin()) {
        gvp.getRenderer()->draw(gvp.getViewport(), gvp.getProjection(), gvp.getView(), dl);
        fb_silhouette.reinitBuffers(gvp.getViewport()->getWidth(), gvp.getViewport()->getHeight());
        fb_outline.reinitBuffers(gvp.getViewport()->getWidth(), gvp.getViewport()->getHeight());
        fb_blur.reinitBuffers(gvp.getViewport()->getWidth(), gvp.getViewport()->getHeight());
        fb_pick.reinitBuffers(gvp.getViewport()->getWidth(), gvp.getViewport()->getHeight());

        gvp.getRenderer()->drawSilhouettes(&fb_silhouette, gvp.getProjection(), gvp.getView(), dl_silhouette);
        outline(&fb_outline, fb_silhouette.getTextureId(0));
        cutout(&fb_blur, fb_outline.getTextureId(0), fb_silhouette.getTextureId(0));/*
        blur(&fb_outline, fb_silhouette.getTextureId(0), gfxm::vec2(1, 0));
        blur(&fb_blur, fb_outline.getTextureId(0), gfxm::vec2(0, 1));
        blur(&fb_outline, fb_blur.getTextureId(0), gfxm::vec2(1, 0));
        blur(&fb_blur, fb_outline.getTextureId(0), gfxm::vec2(0, 1));
        cutout(&fb_outline, fb_blur.getTextureId(0), fb_silhouette.getTextureId(0));*/
        overlay(gvp.getViewport()->getFinalBuffer(), fb_blur.getTextureId(0));
        
        auto mpos = gvp.getMousePos();
        if(gvp.isMouseClicked(0)) {
            if(cur_tweak_stage_id < spawn_tweak_stage_stack.size()) {
                ++cur_tweak_stage_id;
                if(cur_tweak_stage_id == spawn_tweak_stage_stack.size()) {
                    cur_tweak_stage_id = 0;
                    spawn_tweak_stage_stack.clear();
                }
            } else {
                gvp.getRenderer()->drawPickPuffer(&fb_pick, gvp.getProjection(), gvp.getView(), dl);
                
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
    }
    gvp.end();

    cursor_data.normal = gvp.getCursor3dNormal();
    cursor_data.pos = gvp.getCursor3d();
    cursor_data.xy_plane_pos = gvp.getCursorXYPlane();
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

                spawn_tweak_stage_stack.push_back(SpawnTweakStage(
                    ecsEntityHandle(cur_world, selected_ent), cursor_data, [](ecsEntityHandle e, const SpawnTweakData& c){
                        e.getAttrib<ecsTranslation>()->setPosition(c.cursor.pos + c.cursor.normal * 0.5f);
                        return c.cursor_start;
                    }
                ));
            }
        }
        ImGui::EndDragDropTarget();
    }
}


void imguiEntityList(
    ecsWorld* cur_world, 
    const std::vector<entity_id>& entities,
    const std::map<entity_id, std::vector<entity_id>>& child_map,
    entity_id& selected_ent
);

void imguiEntityListItemContextMenu(const char* string_id, ecsEntityHandle hdl, entity_id& selected_ent) {
    if(ImGui::BeginPopupContextItem(string_id)) {
        ImGui::Text(string_id);
        ImGui::Separator();
        if(ImGui::MenuItem("Edit template...")) {

        }
        if(ImGui::MenuItem("Save as template...")) {
            // TODO:
        }
        ImGui::Separator();
        if(ImGui::MenuItem("Delete")) {
            if(selected_ent == hdl.getId()) {
                selected_ent = 0;
            }
            hdl.remove();
        }
        ImGui::EndPopup();
    }
}

static void imguiEntityListItem(
    ecsEntityHandle hdl, 
    const std::string& name, 
    entity_id& selected_ent, 
    const std::map<entity_id, std::vector<entity_id>>& child_map
) {
    bool selected = selected_ent == hdl.getId();
    
    auto it = child_map.find(hdl.getId());
    std::string string_id = MKSTR(name << "###" << hdl.getId());
    if(it == child_map.end()) {
        if(ImGui::Selectable(string_id.c_str(), selected_ent == hdl.getId())) {
            selected_ent = hdl.getId();
        }
        imguiEntityListItemContextMenu(string_id.c_str(), hdl, selected_ent);
    } else { 
        ImGuiTreeNodeFlags tree_node_flags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
        if(selected) {
            tree_node_flags |= ImGuiTreeNodeFlags_Selected;
        }
        bool open = ImGui::TreeNodeEx(MKSTR(name << "###" << hdl.getId()).c_str(), tree_node_flags);
        imguiEntityListItemContextMenu(string_id.c_str(), hdl, selected_ent);
        if(ImGui::IsItemClicked(0)) {
            selected_ent = hdl.getId();
        }
        if(open) {
            imguiEntityList(hdl.getWorld(), it->second, child_map, selected_ent);
            ImGui::TreePop();
        }
    }
}

static void imguiEntityList(
    ecsWorld* cur_world, 
    const std::vector<entity_id>& entities,
    const std::map<entity_id, std::vector<entity_id>>& child_map,
    entity_id& selected_ent
) {
    std::vector<ecsEntityHandle> anon_entities;
    std::vector<ecsEntityHandle> unnamed_entities;
    std::vector<ecsEntityHandle> named_entities;
    
    for(auto e : entities) {
        ecsEntityHandle hdl(cur_world, e);
        ecsParentTransform* pt = hdl.findAttrib<ecsParentTransform>();
        ecsName* name = hdl.findAttrib<ecsName>();
        if(name) {
            if(!name->name.empty()) {
                named_entities.emplace_back(hdl);
            } else {
                unnamed_entities.emplace_back(hdl);
            }
        } else {
            anon_entities.emplace_back(hdl);
        }
    }

    std::sort(anon_entities.begin(), anon_entities.end(), [](const ecsEntityHandle& left, const ecsEntityHandle& right)->bool {
        return left.getId() < right.getId();
    });
    std::sort(unnamed_entities.begin(), unnamed_entities.end(), [](const ecsEntityHandle& left, const ecsEntityHandle& right)->bool {
        return left.getId() < right.getId();
    });
    std::sort(named_entities.begin(), named_entities.end(), [](const ecsEntityHandle& left, const ecsEntityHandle& right)->bool {
        return left.findAttrib<ecsName>()->name < right.findAttrib<ecsName>()->name;
    });

    for(int i = 0; i < named_entities.size(); ++i) {
        auto hdl = named_entities[i];
        std::string name = hdl.findAttrib<ecsName>()->name;
        imguiEntityListItem(hdl, name, selected_ent, child_map);
    }
    for(int i = 0; i < unnamed_entities.size(); ++i) {
        auto hdl = unnamed_entities[i];
        imguiEntityListItem(hdl, "[EMPTY_NAME]", selected_ent, child_map);
    }
    for(int i = 0; i < anon_entities.size(); ++i) {
        auto hdl = anon_entities[i];
        imguiEntityListItem(hdl, "[ANONYMOUS]", selected_ent, child_map);
    }
}

void DocEcsWorld::onGuiToolbox(Editor* ed) {
    if(!subscene_stack.empty()) {
        cur_world = subscene_stack.back();
    } else {
        cur_world = _resource.get();
    }

    for(int i = 0; i < cur_world->systemCount(); ++i) {
        auto sys = cur_world->getSystem(i);
        bool exists = true;
        if(ImGui::CollapsingHeader(MKSTR( sys ).c_str(), &exists, ImGuiTreeNodeFlags_DefaultOpen)) {
            
        }
    }

    if(ImGui::SmallButton(ICON_MDI_PLUS " Create")) {
        selected_ent = cur_world->createEntity().getId();
        ecsEntityHandle h(cur_world, selected_ent);
        h.getAttrib<ecsName>()->name = "New object";
        h.getAttrib<ecsWorldTransform>();
        h.getAttrib<ecsTranslation>();
        h.getAttrib<ecsRotation>();
        h.getAttrib<ecsScale>();
    }
    ImGui::SameLine();
    if(ImGui::SmallButton(ICON_MDI_MINUS " Remove")) {
        cur_world->removeEntity(selected_ent);
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
        auto& entities = cur_world->getEntities();
        for(auto e : entities) {
            ecsEntityHandle hdl(cur_world, e);
            ecsParentTransform* pt = hdl.findAttrib<ecsParentTransform>();
            if(pt && pt->parent_entity >= 0) {
                child_map[pt->parent_entity].emplace_back(hdl.getId());
                continue;
            }
            top_level_entities.emplace_back(hdl.getId());
        }
        
        if(search_query.empty()) {
            imguiEntityList(cur_world, top_level_entities, child_map, selected_ent);
        } else {
            std::vector<entity_id> found_entities;
            auto& entities = cur_world->getEntities();
            for(auto e : entities) {
                ecsEntityHandle hdl(cur_world, e);
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
            imguiEntityList(cur_world, found_entities, child_map, selected_ent);
        }
        ImGui::ListBoxFooter();
    }
    ImGui::PopItemWidth();

    // TODO: Check selected entity validity
    ImGui::Text(MKSTR("UID: " << selected_ent).c_str());

    ImGui::PushItemWidth(-1);
    if(ImGui::BeginCombo("###ADD_ATTRIBUTE", ICON_MDI_PLUS " Add attribute...")) {
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

void DocEcsWorld::onFocus() {
    kt_cmd_set_callback("ecs_world_subdoc", std::bind(&DocEcsWorld::onCmdSubdoc, this, std::placeholders::_1, std::placeholders::_2));
}
void DocEcsWorld::onUnfocus() {
    kt_cmd_clear_callback("ecs_world_subdoc");
}

void DocEcsWorld::onCmdSubdoc(int argc, const char* argv[]) {
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