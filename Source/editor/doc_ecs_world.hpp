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

#include "../util/lightmap/gen_lightmaps.hpp"


struct CursorData {
    gfxm::vec2 posScreen;
    gfxm::vec3 pos;
    gfxm::vec3 xy_plane_pos;
    gfxm::vec3 normal;
};

struct SpawnTweakData {
    CursorData cursor;
    CursorData cursor_start;

    gfxm::vec2 screen_delta;
    gfxm::vec3 world_delta;
    gfxm::vec3 plane_xy_delta;
};


typedef CursorData(*spawn_tweak_cb_t)(ecsEntityHandle, const SpawnTweakData&);

class SpawnTweakStage {
    ecsEntityHandle e;
    SpawnTweakData spawn_tweak_data;
    spawn_tweak_cb_t cb;

public:
    SpawnTweakStage(ecsEntityHandle e, const CursorData& base_cursor, spawn_tweak_cb_t cb)
    : e(e), cb(cb) {
        spawn_tweak_data.cursor_start = base_cursor;
    }

    void updateCursorData(const CursorData& cursor_data) {
        spawn_tweak_data.cursor = cursor_data;
        spawn_tweak_data.world_delta = cursor_data.pos - spawn_tweak_data.cursor_start.pos;
        spawn_tweak_data.screen_delta = cursor_data.posScreen - spawn_tweak_data.cursor_start.posScreen;
        spawn_tweak_data.plane_xy_delta = cursor_data.xy_plane_pos - spawn_tweak_data.cursor_start.xy_plane_pos;
    }

    void setBaseCursor(const CursorData& cd) {
        spawn_tweak_data.cursor_start = cd;
    }

    CursorData update(const CursorData& cursor_data) {
        updateCursorData(cursor_data);
        return update();
    }

    CursorData update() {
        return cb(e, spawn_tweak_data);
    }
};


#include "util/editor_task_mgr.hpp"

class edTaskEcsWorldModelDragNDrop : public edTaskAsync {
public:
    std::string entity_name;
    std::string file_path;
    ecsSubScene subscene;
    ecsWorld*   target_world;

    edTaskEcsWorldModelDragNDrop(const char* name, const char* file_path, const char* entity_name, ecsWorld* target_world)
        : edTaskAsync(name), file_path(file_path), entity_name(entity_name), target_world(target_world)
    {
        
    }

    void execute() override {
        subscene = ecsSubScene(std::shared_ptr<ecsWorld>(new ecsWorld()));
        ecsysSceneGraph* sceneGraph = subscene.getWorld()->getSystem<ecsysSceneGraph>();
        assimpImportEcsScene(sceneGraph, file_path.c_str());
    }
};

class edTaskEcsWorldCalcLightmaps : public edTaskAsync {
public:
    ecsRenderSystem* renderSys = 0;
    GuiViewport* gvp = 0; // Ugh
    DrawList dl;

    edTaskEcsWorldCalcLightmaps(const char* name)
        : edTaskAsync(name)
    {
        
    }

    void execute() override {
        auto entities = renderSys->get_array<ecsTuple<ecsWorldTransform, ecsMeshes>>();
        std::vector<LightmapMeshData> mesh_data;
        delegatedCall([&entities, &mesh_data](){
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
        });

        // TODO: USE SEPARATE GBUFFER WTF
        GenLightmaps(mesh_data, gvp->getRenderer(), gvp->getViewport()->getGBuffer(), dl);

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
};

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

    CursorData                        cursor_data;
    std::vector<SpawnTweakStage>      spawn_tweak_stage_stack;
    int                               cur_tweak_stage_id = 0;

    std::set<std::unique_ptr<edTaskEcsWorldModelDragNDrop>> model_dnd_tasks;
    std::unique_ptr<edTaskEcsWorldCalcLightmaps> lightmap_task;

public:
    DocEcsWorld();

    void onResourceSet() override;

    void onGui(Editor* ed, float dt) override;
    void onGuiToolbox(Editor* ed) override;

    void onFocus() override;
    void onUnfocus() override;

    void onCmdSubdoc(int argc, const char* argv[]);

};
STATIC_RUN(DocEcsWorld) {
    regEditorDocument<DocEcsWorld>({ "ecsw" });
}

#endif
