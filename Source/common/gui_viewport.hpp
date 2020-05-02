#ifndef IMGUI_VIEWPORT_HPP
#define IMGUI_VIEWPORT_HPP

#include "../common/gfxm.hpp"
#include "../common/lib/imgui_wrap.hpp"
#include "../common/render_viewport.hpp"
#include "../common/renderer.hpp"
#include "../common/debug_draw.hpp"

#include "../common/util/object_set.hpp"

class GameScene;
class ktNode;

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
    gl::FrameBuffer fb_outline;
    gl::FrameBuffer fb_blur;
    gl::FrameBuffer fb_silhouette;
    gl::FrameBuffer fb_pick;
    std::vector<float> readback_buffer;

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
    gfxm::vec3  cursor_3d;
    gfxm::vec3  cursor_3d_normal;
    gfxm::vec3  cursor_xy_plane;

    //
    gfxm::ivec2 pos;
    gfxm::vec2 viewport_sz;

    gfxm::mat4 _proj;
    gfxm::mat4 _view;

    bool gui_visible = false;
    ImRect bb;
    ImVec2 vp_sz;
    
public:
    GuiViewport();
    ~GuiViewport();

    RendererPBR* getRenderer() { return &renderer; }
    RenderViewport* getViewport() { return &rvp; }
    DebugDraw& getDebugDraw();

    gfxm::ivec2 getPos() const;
    gfxm::ivec2 getSize() const;
    gfxm::mat4 getProjection() const;
    gfxm::mat4 getView() const;
    bool isMouseClicked(int button);
    gfxm::ivec2 getMousePos();
    gfxm::vec3 getMouseScreenToWorldPos(float height);

    gfxm::vec3 getCursorXYPlane();
    gfxm::vec3 getCursor3d();
    gfxm::vec3 getCursor3dNormal();

    bool debugDrawEnabled() const;
    void enableDebugDraw(bool v);
    void resetCamera(gfxm::vec3 focus, float zoom);
    void camMode(CAM_MODE mode);
    void camMove(gfxm::vec2 v);
    void camTranslate(gfxm::vec2 v);
    void camRotate(gfxm::vec2 v);
    void camZoom(float v);
    void camSetPivot(const gfxm::vec3& pivot);

    void draw(DrawList& dl, gfxm::ivec2 sz = gfxm::ivec2(0,0));
    void draw(GameScene* scn, ObjectSet* selected_objects = 0, gfxm::ivec2 sz = gfxm::ivec2(0,0));

    bool begin(gfxm::ivec2 sz = gfxm::ivec2(0,0));
    void end();
};

void blur(gl::FrameBuffer* fb, GLuint tex_0, const gfxm::vec2& dir);
void cutout(gl::FrameBuffer* fb, GLuint tex_0, GLuint tex_1);
void overlay(gl::FrameBuffer* fb, GLuint texId);

#endif
