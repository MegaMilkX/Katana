#include "gui_viewport.hpp"

#include "../common/scene/game_scene.hpp"
#include "../common/scene/controllers/render_controller.hpp"

GuiViewport::GuiViewport() {
    rvp.init(640, 480);
    dd.init();
    memset(mouse_clicked, 0, sizeof(mouse_clicked));
}
GuiViewport::~GuiViewport() {
    dd.cleanup();
}

bool GuiViewport::isMouseClicked(int button) {
    if(button > 4) return false;
    return mouse_clicked[button];
}

gfxm::ivec2 GuiViewport::getMousePos() {
    return mouse_pos;
}

gfxm::vec3 GuiViewport::getMouseScreenToWorldPos(float height) {
    gfxm::mat4 proj = gfxm::perspective(gfxm::radian(45.0f), viewport_sz.x/(float)viewport_sz.y, 0.01f, 1000.0f);
        gfxm::transform tcam;
        cam_pos = gfxm::lerp(cam_pos, cam_pivot, 0.2f);
        cam_zoom_actual = gfxm::lerp(cam_zoom_actual, cam_zoom, 0.2f);
        tcam.position(cam_pos);
        tcam.rotate(cam_angle_y, gfxm::vec3(0.0f, 1.0f, 0.0f));
        tcam.rotate(cam_angle_x, tcam.right());
        tcam.translate(tcam.back() * cam_zoom_actual);
        gfxm::mat4 view = gfxm::inverse(tcam.matrix());

    return gfxm::screenToWorldPlaneXY(
        gfxm::vec2(mouse_pos.x, mouse_pos.y),
        viewport_sz,
        proj, view
    );
}

DebugDraw& GuiViewport::getDebugDraw() {
    return dd;
}

bool GuiViewport::debugDrawEnabled() const {
    return debug_draw_enabled;
}
void GuiViewport::enableDebugDraw(bool v) {
    debug_draw_enabled = v;
}
void GuiViewport::resetCamera(gfxm::vec3 focus, float zoom) {
    cam_zoom = zoom;
    cam_pivot = focus;
}
void GuiViewport::camMode(CAM_MODE mode) {
    cam_mode = mode;
}
void GuiViewport::camMove(gfxm::vec2 v) {
    if(cam_mode == CAM_PAN) {
        camTranslate(v);
    } else if(cam_mode == CAM_ORBIT) {
        camRotate(gfxm::vec2(v.y, v.x));
    }
}
void GuiViewport::camTranslate(gfxm::vec2 v) {
    gfxm::transform tcam;
    tcam.rotate(cam_angle_y, gfxm::vec3(0.0f, 1.0f, 0.0f));
    tcam.rotate(cam_angle_x, tcam.right());
    float mod = cam_zoom;
    gfxm::vec3 right = tcam.right();
    gfxm::vec3 up = tcam.up();
    cam_pivot += right * -v.x * 0.01f * mod * 0.15f;
    cam_pivot += up * v.y * 0.01f * mod * 0.15f;
}
void GuiViewport::camRotate(gfxm::vec2 v) {
    cam_angle_x += -v.x * 0.01f;
    cam_angle_y += -v.y * 0.01f;
}
void GuiViewport::camZoom(float v) {
    float mod = cam_zoom;
    cam_zoom += -v * mod * 0.15f;
}

void GuiViewport::draw(GameScene* scn, GameObject* selected_object, gfxm::ivec2 sz) {        
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

        if(selected_object && scn == selected_object->getScene()) {
            selected_object->onGizmo(*this);
        }

        auto& io = ImGui::GetIO();
        is_mouse_over = ImGui::IsWindowHovered();
        window_in_focus = ImGui::IsRootWindowFocused();

        auto window = ImGui::GetCurrentWindow();
        auto bb = window->ClipRect;
        auto vp_sz = ImVec2(bb.Max.x - bb.Min.x, bb.Max.y - bb.Min.y);

        viewport_sz = gfxm::vec2(
            vp_sz.x, vp_sz.y
        );

        // === Input ==================
        for(int i = 0; i < 5; ++i) {
            mouse_clicked[i] = is_mouse_over && ImGui::IsMouseClicked(i);
        }
        if(is_mouse_over) {
            //  mouse_pos
            ImVec2 impos = ImGui::GetMousePos();
            mouse_pos.x = impos.x - bb.Min.x;
            mouse_pos.y = impos.y - bb.Min.y;
            mouse_pos.y = vp_sz.y - mouse_pos.y; 
        }
        // ============================

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