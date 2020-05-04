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
