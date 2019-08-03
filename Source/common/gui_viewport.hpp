#ifndef IMGUI_VIEWPORT_HPP
#define IMGUI_VIEWPORT_HPP

#include "../common/gfxm.hpp"
#include "../common/lib/imgui_wrap.hpp"
#include "../common/render_viewport.hpp"
#include "../common/renderer.hpp"
#include "../common/debug_draw.hpp"

class GameScene;
class GameObject;

class GuiViewport {
public:
    enum CAM_MODE {
        CAM_PAN,
        CAM_ORBIT
    };
private:
    RenderViewport rvp;
    RendererPBR renderer;
    DebugDraw dd;

    CAM_MODE cam_mode = CAM_PAN;
    bool debug_draw_enabled = true;

    bool mouse_look = false;
    bool mouse_look_alt = false;
    float cam_angle_y = gfxm::radian(45.0f);
    float cam_angle_x = gfxm::radian(-25.0f);
    float cam_zoom = 5.0f;
    gfxm::vec3 cam_pivot;

    gfxm::vec3 cam_pos;
    float cam_zoom_actual = 0.0f;
    float cam_angle_y_actual = gfxm::radian(45.0f);
    float cam_angle_x_actual = gfxm::radian(-25.0f);

    bool is_mouse_over = false;
    bool window_in_focus = false;

    ImVec2 drag_delta_prev = ImVec2(0,0);
    bool mouse_captured = false;

    // Input
    bool mouse_clicked[5];
    gfxm::ivec2 mouse_pos;

    //
    gfxm::ivec2 pos;
    gfxm::vec2 viewport_sz;

    gfxm::mat4 _proj;
    gfxm::mat4 _view;
public:
    GuiViewport();
    ~GuiViewport();

    gfxm::ivec2 getPos() const;
    gfxm::ivec2 getSize() const;
    gfxm::mat4 getProjection() const;
    gfxm::mat4 getView() const;
    bool isMouseClicked(int button);
    gfxm::ivec2 getMousePos();
    gfxm::vec3 getMouseScreenToWorldPos(float height);

    DebugDraw& getDebugDraw();

    bool debugDrawEnabled() const;
    void enableDebugDraw(bool v);
    void resetCamera(gfxm::vec3 focus, float zoom);
    void camMode(CAM_MODE mode);
    void camMove(gfxm::vec2 v);
    void camTranslate(gfxm::vec2 v);
    void camRotate(gfxm::vec2 v);
    void camZoom(float v);

    void draw(GameScene* scn, GameObject* selected_object = 0, gfxm::ivec2 sz = gfxm::ivec2(0,0));
};

#endif
