#ifndef IMGUI_VIEWPORT_HPP
#define IMGUI_VIEWPORT_HPP

#include "render_viewport.hpp"
#include "renderer.hpp"
#include "scene/game_scene.hpp"
#include "scene/controllers/render_controller.hpp"

class GuiViewport {
public:
    enum CAM_MODE {
        CAM_PAN,
        CAM_ORBIT
    };
private:
    RenderViewport rvp;
    Renderer renderer;
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

    bool is_mouse_over = false;
    bool window_in_focus = false;

    ImVec2 drag_delta_prev = ImVec2(0,0);
    bool mouse_captured = false;
public:
    GuiViewport() {
        rvp.init(640, 480);
        dd.init();
    }
    ~GuiViewport() {
        dd.cleanup();
    }

    bool debugDrawEnabled() const {
        return debug_draw_enabled;
    }
    void enableDebugDraw(bool v) {
        debug_draw_enabled = v;
    }
    void resetCamera(gfxm::vec3 focus, float zoom) {
        cam_zoom = zoom;
        cam_pivot = focus;
    }
    void camMode(CAM_MODE mode) {
        cam_mode = mode;
    }
    void camMove(gfxm::vec2 v) {
        if(cam_mode == CAM_PAN) {
            camTranslate(v);
        } else if(cam_mode == CAM_ORBIT) {
            camRotate(gfxm::vec2(v.y, v.x));
        }
    }
    void camTranslate(gfxm::vec2 v) {
        gfxm::transform tcam;
        tcam.rotate(cam_angle_y, gfxm::vec3(0.0f, 1.0f, 0.0f));
        tcam.rotate(cam_angle_x, tcam.right());
        float mod = cam_zoom;
        gfxm::vec3 right = tcam.right();
        gfxm::vec3 up = tcam.up();
        cam_pivot += right * -v.x * 0.01f * mod * 0.15f;
        cam_pivot += up * v.y * 0.01f * mod * 0.15f;
    }
    void camRotate(gfxm::vec2 v) {
        cam_angle_x += -v.x * 0.01f;
        cam_angle_y += -v.y * 0.01f;
    }
    void camZoom(float v) {
        float mod = cam_zoom;
        cam_zoom += -v * mod * 0.15f;
    }

    void draw(GameScene* scn, gfxm::ivec2 sz = gfxm::ivec2(0,0)) {        
        if(ImGui::BeginChildFrame(0, ImVec2(sz.x, sz.y))) {
            dd.clear();
            dd.line(gfxm::vec3(-11.0f, .0f, -11.0f), gfxm::vec3(-10.0, .0f, -11.0f), gfxm::vec3(1.0f, .0f, .0f));
            dd.line(gfxm::vec3(-11.0f, .0f, -11.0f), gfxm::vec3(-11.0, 1.0f, -11.0f), gfxm::vec3(.0f, 1.0f, .0f));
            dd.line(gfxm::vec3(-11.0f, .0f, -11.0f), gfxm::vec3(-11.0, .0f, -10.0f), gfxm::vec3(.0f, .0f, 1.0f));
            dd.gridxz(
                gfxm::vec3(-10.0f, .0f, -10.0f),
                gfxm::vec3(10.0f, .0f, 10.0f),
                1,
                gfxm::vec3(0.2f, 0.2f, 0.2f)
            );
            /*
            if(editor->getSelectedObject()) {
                dd.aabb(
                    editor->getSelectedObject()->getAabb(),
                    gfxm::vec3(1.0f, 1.0f, 1.0f)
                );
            }*/

            auto& io = ImGui::GetIO();
            is_mouse_over = ImGui::IsWindowHovered();
            window_in_focus = ImGui::IsRootWindowFocused();

            auto window = ImGui::GetCurrentWindow();
            auto bb = window->ClipRect;
            auto vp_sz = ImVec2(bb.Max.x - bb.Min.x, bb.Max.y - bb.Min.y);

            rvp.resize(vp_sz.x, vp_sz.y);
            gfxm::mat4 proj = gfxm::perspective(gfxm::radian(45.0f), vp_sz.x/(float)vp_sz.y, 0.01f, 1000.0f);
            gfxm::transform tcam;
            cam_pos = gfxm::lerp(cam_pos, cam_pivot, 0.2f);
            cam_zoom_actual = gfxm::lerp(cam_zoom_actual, cam_zoom, 0.2f);
            tcam.position(cam_pos);
            tcam.rotate(cam_angle_y, gfxm::vec3(0.0f, 1.0f, 0.0f));
            tcam.rotate(cam_angle_x, tcam.right());
            tcam.translate(tcam.back() * cam_zoom_actual);
            gfxm::mat4 view = gfxm::inverse(tcam.matrix());

            DrawList dl;
            scn->getController<RenderController>()->getDrawList(dl);
            renderer.draw(&rvp, proj, view, dl);
            
            rvp.getFinalBuffer()->bind();
            glViewport(0, 0, (GLsizei)vp_sz.x, (GLsizei)vp_sz.y);
            if(debug_draw_enabled) {
                dd.draw(proj, view);
            }

            ImGui::GetWindowDrawList()->AddImage((void*)rvp.getFinalImage(),
                bb.Min,
                bb.Max,
                ImVec2(0, 1),
                ImVec2(1, 0)
            );

            if(ImGui::IsMouseClicked(2) && is_mouse_over) {
                mouse_captured = true;
            }
            if(ImGui::IsMouseDragging(2) && mouse_captured) {
                // TODO
                ImVec2 d = ImGui::GetMouseDragDelta(2) - drag_delta_prev;
                camMove(gfxm::vec2(d.x, d.y)); 

                //ImGui::Text(MKSTR(gfxm::vec2(d.x, d.y)).c_str());

                drag_delta_prev = ImGui::GetMouseDragDelta(2);
            }
            if(ImGui::IsMouseReleased(2)) {
                drag_delta_prev = ImVec2(0,0);
                mouse_captured = false;
            }
            if(is_mouse_over) {
                camZoom(io.MouseWheel);
            }

            ImGui::EndChildFrame();
        }
    }
};

#endif
