#ifndef DOC_ECS_WORLD_MODE_HPP
#define DOC_ECS_WORLD_MODE_HPP

#include "../../common/ecs/world.hpp"

#include "../../common/ecs/attribs/base_attribs.hpp"

#include "../../common/ecs/systems/dynamics.hpp"
#include "../../common/ecs/systems/render.hpp"
#include "../../common/ecs/systems/render_gui.hpp"

#include "../../common/renderer.hpp"
#include "../../common/gui_viewport.hpp"

#include "../common/util/font/font.hpp"

#include "gui_helpers.hpp"


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


static const int UNDO_STACK_MAX = 35;

struct DocEcsWorldState {
    typedef std::vector<std::shared_ptr<ecsWorld>> undo_stack_t;

    DocEcsWorldState::undo_stack_t* undo_stack = 0;

    ecsWorld*           world = 0;
    entity_id           selected_ent = 0;
    GuiViewport         gvp;
    DrawList            dl;

    void backupState() {
        assert(undo_stack);
        if(undo_stack->size() == UNDO_STACK_MAX) {
            undo_stack->erase(undo_stack->begin());
        }

        undo_stack->push_back(std::shared_ptr<ecsWorld>(new ecsWorld()));
        dstream strm;
        world->serialize(strm);
        strm.jump(0);
        undo_stack->back()->deserialize(strm, strm.bytes_available());
    }

    void restoreState() {
        assert(undo_stack);
        if(undo_stack->empty()) {
            return;
        }

        dstream strm;
        undo_stack->back()->serialize(strm);
        undo_stack->pop_back();
        strm.jump(0);
        world->clearEntities();
        world->deserialize(strm, strm.bytes_available());

        selected_ent = 0; // TODO: Grab selected entity from world's editor payload (when it's done)
    }
};

class DocEcsWorldMode {
public:
    virtual ~DocEcsWorldMode() {}

    virtual const char* getName() const = 0;

    virtual void onMenuBar(DocEcsWorldState& state) {}
    virtual void onMainWindow(DocEcsWorldState& state) {}
    virtual void onToolbox(DocEcsWorldState& state) {}
};


#endif
