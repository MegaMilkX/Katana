#ifndef EDITOR_VIEWPORT_HPP
#define EDITOR_VIEWPORT_HPP

#include "scene/game_scene.hpp"

#include "render_viewport.hpp"
#include "renderer.hpp"

#include "../common/input/input_mgr.hpp"

#include "../common/debug_draw.hpp"

#include "gfx_scene_mgr.hpp"
#include "anim_scene_mgr.hpp"

class Editor;
class EditorViewport {
public:
    EditorViewport();
    ~EditorViewport();
    void init(Editor* editor, GameScene* scene);
    void update(Editor* editor);
private:
    Editor* editor;
    RenderViewport vp;
    Renderer renderer;
    GfxSceneMgr gfx_mgr;
    AnimationSceneMgr anim_mgr;

    DebugDraw dd;
    InputListener* input_lis = 0;
};

#endif
