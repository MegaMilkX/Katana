#ifndef DOC_ECS_WORLD_MODE_HPP
#define DOC_ECS_WORLD_MODE_HPP

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

#include "state.hpp"

class DocEcsWorldMode {
public:
    virtual ~DocEcsWorldMode() {}

    virtual const char* getName() const = 0;

    virtual void onMenuBar(DocEcsWorldState& state) {}
    virtual void onMainWindow(DocEcsWorldState& state) {}
    virtual void onToolbox(DocEcsWorldState& state) {}
};


#endif
