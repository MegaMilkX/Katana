#ifndef EDITOR_VIEWPORT_HPP
#define EDITOR_VIEWPORT_HPP

#include "scene/game_scene.hpp"

#include "../common/render_viewport.hpp"
#include "../common/renderer.hpp"

#include "../common/input/input_mgr.hpp"

#include "../common/debug_draw.hpp"

#include "octree.hpp"

#include "gui_viewport.hpp"

class Editor;
class EditorViewport {
public:
    EditorViewport();
    ~EditorViewport();
    void init(Editor* editor, GameScene* scene);
    void update(Editor* editor);
    
    void recenterCamera();
    GuiViewport& getViewport();
private:
    GuiViewport gvp;

    Editor* editor;
    RenderViewport vp;
    Renderer renderer;

    DebugDraw dd;
    InputListener* input_lis = 0;

    Octree oct;
    OctreeObject* oct_o;

    bool mouse_is_over_vp = false;
    bool window_in_focus = false;
};

#endif
