#include "gui_viewport.hpp"

#include "../common/scene/game_scene.hpp"
#include "../common/scene/controllers/render_controller.hpp"
#include "../common/scene/controllers/dynamics_ctrl.hpp"

#include "../common/lib/imguizmo/ImGuizmo.h"

#include "../common/attributes/render_environment.hpp"

#include "../common/render/shader_loader.hpp"

void outline(gl::FrameBuffer* fb, GLuint texId) {
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, fb->getWidth(), fb->getHeight());
    static gl::ShaderProgram* prog = shaderLoader().loadShaderProgram("shaders/outline.glsl");

    fb->bind();
    glClear(GL_COLOR_BUFFER_BIT);
    prog->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texId);
    drawQuad();
}

void blur(gl::FrameBuffer* fb, GLuint tex_0, const gfxm::vec2& dir) {
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, fb->getWidth(), fb->getHeight());
    static gl::ShaderProgram* prog = shaderLoader().loadShaderProgram("shaders/blur.glsl");

    fb->bind();
    glClear(GL_COLOR_BUFFER_BIT);
    prog->use();
    glUniform2fv(prog->getUniform("u_dir"), 1, (float*)&dir);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_0);
    drawQuad();
}
void cutout(gl::FrameBuffer* fb, GLuint tex_0, GLuint tex_1) {
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, fb->getWidth(), fb->getHeight());
    static gl::ShaderProgram* prog = shaderLoader().loadShaderProgram("shaders/cutout.glsl");

    fb->bind();
    glClear(GL_COLOR_BUFFER_BIT);
    prog->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tex_1);
    drawQuad();
}

void overlay(gl::FrameBuffer* fb, GLuint texId) {
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, fb->getWidth(), fb->getHeight());
    static gl::ShaderProgram* prog = shaderLoader().loadShaderProgram("shaders/overlay.glsl");

    fb->bind();
    prog->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texId);
    drawQuad();
}


GuiViewport::GuiViewport() {
    rvp.init(640, 480);
    dd.init();

    fb_silhouette.addBuffer(0, GL_RED, GL_UNSIGNED_BYTE);
    fb_outline.addBuffer(0, GL_RED, GL_UNSIGNED_BYTE);
    fb_blur.addBuffer(0, GL_RED, GL_UNSIGNED_BYTE);
    fb_pick.addBuffer(0, GL_RGB, GL_UNSIGNED_INT);

    memset(mouse_clicked, 0, sizeof(mouse_clicked));

    readback_buffer.resize(640 * 480 * 4);
}
GuiViewport::~GuiViewport() {
    dd.cleanup();
}

gfxm::ivec2 GuiViewport::getPos() const {
    return pos;
}
gfxm::ivec2 GuiViewport::getSize() const {
    return gfxm::ivec2(viewport_sz.x, viewport_sz.y);
}
gfxm::mat4 GuiViewport::getProjection() const {
    return _proj;
}
gfxm::mat4 GuiViewport::getView() const {
    return _view;
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
    cam_angle_x_actual = gfxm::lerp(cam_angle_x_actual, cam_angle_x, 0.5f);
    cam_angle_y_actual = gfxm::lerp(cam_angle_y_actual, cam_angle_y, 0.5f);
    tcam.rotate(cam_angle_y_actual, gfxm::vec3(0.0f, 1.0f, 0.0f));
    tcam.rotate(cam_angle_x_actual, tcam.right());
    tcam.translate(tcam.back() * cam_zoom_actual);
    gfxm::mat4 view = gfxm::inverse(tcam.matrix());

    return gfxm::screenToWorldPlaneXY(
        gfxm::vec2(mouse_pos.x, mouse_pos.y),
        viewport_sz,
        proj, view
    );
}

gfxm::vec3 GuiViewport::getCursorXYPlane() {
    return cursor_xy_plane;
}
gfxm::vec3 GuiViewport::getCursor3d() {
    return cursor_3d;
}
gfxm::vec3 GuiViewport::getCursor3dNormal() {
    return cursor_3d_normal;
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
    if(mod < FLT_EPSILON) mod = 0.0001f;
    cam_zoom += -v * mod * 0.15f;
}
void GuiViewport::camSetPivot(const gfxm::vec3& pivot) {
    cam_pivot = pivot;
    cam_pos = pivot;
}

#include "util/mesh_gen.hpp"
#include "lib/imguizmo/ImGuizmo.h"

void GuiViewport::draw(DrawList& dl, gfxm::ivec2 sz) {
    if(ImGui::BeginChild(ImGui::GetID(this), ImVec2(sz.x, sz.y))) {
        auto window = ImGui::GetCurrentWindow();
        auto bb = window->ClipRect;

        auto& io = ImGui::GetIO();
        is_mouse_over = ImGui::IsWindowHovered();
        window_in_focus = ImGui::IsRootWindowFocused();
        
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

        dd.line(gfxm::vec3(-11.0f, .0f, -11.0f), gfxm::vec3(-10.0, .0f, -11.0f), gfxm::vec3(1.0f, .0f, .0f));
        dd.line(gfxm::vec3(-11.0f, .0f, -11.0f), gfxm::vec3(-11.0, 1.0f, -11.0f), gfxm::vec3(.0f, 1.0f, .0f));
        dd.line(gfxm::vec3(-11.0f, .0f, -11.0f), gfxm::vec3(-11.0, .0f, -10.0f), gfxm::vec3(.0f, .0f, 1.0f));
        dd.gridxz(
            gfxm::vec3(-10.0f, .0f, -10.0f),
            gfxm::vec3(10.0f, .0f, 10.0f),
            1,
            gfxm::vec3(0.2f, 0.2f, 0.2f)
        );

        auto vp_sz = ImVec2(bb.Max.x - bb.Min.x, bb.Max.y - bb.Min.y);

        rvp.resize(vp_sz.x, vp_sz.y);
        _proj = gfxm::perspective(gfxm::radian(45.0f), vp_sz.x/(float)vp_sz.y, 0.1f, 1000.0f);
        gfxm::transform tcam;
        cam_pos = gfxm::lerp(cam_pos, cam_pivot, 0.2f);
        cam_zoom_actual = gfxm::lerp(cam_zoom_actual, cam_zoom, 0.2f);
        tcam.position(cam_pos);
        cam_angle_x_actual = gfxm::lerp(cam_angle_x_actual, cam_angle_x, 0.5f);
        cam_angle_y_actual = gfxm::lerp(cam_angle_y_actual, cam_angle_y, 0.5f);
        tcam.rotate(cam_angle_y_actual, gfxm::vec3(0.0f, 1.0f, 0.0f));
        tcam.rotate(cam_angle_x_actual, tcam.right());
        tcam.translate(tcam.back() * cam_zoom_actual);
        _view = gfxm::inverse(tcam.matrix());

        renderer.draw(&rvp, _proj, _view, dl, false, true);

        rvp.getFinalBuffer()->bind();
        glViewport(0, 0, (GLsizei)vp_sz.x, (GLsizei)vp_sz.y);
        if(debug_draw_enabled) {
            dd.draw(_proj, _view);
        }
        dd.clear();

        GLuint buffers[] = {
            rvp.getFinalImage(),
            rvp.getGBuffer()->getAlbedoTexture(),
            rvp.getGBuffer()->getNormalTexture(),
            rvp.getGBuffer()->getRoughnessTexture(),
            rvp.getGBuffer()->getMetallicTexture(),
            rvp.getGBuffer()->getLightnessTexture(),
            rvp.getGBuffer()->getDepthTexture()
        };
        ImGui::GetWindowDrawList()->AddImage((void*)buffers[dbg_renderBufferId],
            bb.Min,
            bb.Max,
            ImVec2(0, 1),
            ImVec2(1, 0)
        );
    }
    ImGui::EndChild();
}

void GuiViewport::draw(GameScene* scn, ObjectSet* selected_objects, gfxm::ivec2 sz) {        
    if(ImGui::BeginChild(ImGui::GetID(this), ImVec2(sz.x, sz.y))) {
        //ImGuizmo::SetDrawlist();

        dd.line(gfxm::vec3(-11.0f, .0f, -11.0f), gfxm::vec3(-10.0, .0f, -11.0f), gfxm::vec3(1.0f, .0f, .0f));
        dd.line(gfxm::vec3(-11.0f, .0f, -11.0f), gfxm::vec3(-11.0, 1.0f, -11.0f), gfxm::vec3(.0f, 1.0f, .0f));
        dd.line(gfxm::vec3(-11.0f, .0f, -11.0f), gfxm::vec3(-11.0, .0f, -10.0f), gfxm::vec3(.0f, .0f, 1.0f));
        dd.gridxz(
            gfxm::vec3(-20.0f, .0f, -20.0f),
            gfxm::vec3(20.0f, .0f, 20.0f),
            1,
            gfxm::vec3(0.04f, 0.04f, 0.04f)
        );

        scn->getController<RenderController>()->debugDraw(getDebugDraw());

        if(scn->hasController<DynamicsCtrl>() && debug_draw_enabled) {
            scn->getController<DynamicsCtrl>()->debugDraw(getDebugDraw());
        }
        /*
        if(editor->getSelectedObject()) {
            dd.aabb(
                editor->getSelectedObject()->getAabb(),
                gfxm::vec3(1.0f, 1.0f, 1.0f)
            );
        }*/

        auto window = ImGui::GetCurrentWindow();
        ImRect crect = window->ContentsRegionRect;
        ImGuizmo::SetRect(crect.Min.x, crect.Min.y, crect.Max.x - crect.Min.x, crect.Max.y - crect.Min.y);

        bool using_gizmo = false;
        if(selected_objects) {
            if(selected_objects->getAll().size() == 1) {
                ktNode* selected = *selected_objects->getAll().begin();
                if(selected->getRoot() == scn) {
                    if(selected->onGizmo(*this)) {
                        using_gizmo = true;
                    }
                }
            }
        }

        auto& io = ImGui::GetIO();
        is_mouse_over = ImGui::IsWindowHovered();
        window_in_focus = ImGui::IsRootWindowFocused();

        bb = window->ClipRect;
        ImVec2 cursor_pos = ImGui::GetCursorPos();
        pos.x = cursor_pos.x;
        pos.y = cursor_pos.y;
        vp_sz = ImVec2(bb.Max.x - bb.Min.x, bb.Max.y - bb.Min.y);

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

        fb_silhouette.reinitBuffers(vp_sz.x, vp_sz.y);
        fb_outline.reinitBuffers(vp_sz.x, vp_sz.y);
        fb_blur.reinitBuffers(vp_sz.x, vp_sz.y);
        fb_pick.reinitBuffers(vp_sz.x, vp_sz.y);

        rvp.resize(vp_sz.x, vp_sz.y);
        _proj = gfxm::perspective(gfxm::radian(45.0f), vp_sz.x/(float)vp_sz.y, 0.1f, 1000.0f);
        gfxm::transform tcam;
        cam_pos = gfxm::lerp(cam_pos, cam_pivot, 0.2f);
        cam_zoom_actual = gfxm::lerp(cam_zoom_actual, cam_zoom, 0.2f);
        tcam.position(cam_pos);
        cam_angle_x_actual = gfxm::lerp(cam_angle_x_actual, cam_angle_x, 0.5f);
        cam_angle_y_actual = gfxm::lerp(cam_angle_y_actual, cam_angle_y, 0.5f);
        tcam.rotate(cam_angle_y_actual, gfxm::vec3(0.0f, 1.0f, 0.0f));
        tcam.rotate(cam_angle_x_actual, tcam.right());
        tcam.translate(tcam.back() * cam_zoom_actual);
        _view = gfxm::inverse(tcam.matrix());

        DrawList dl;
        DrawList dl_silhouettes;
        gfxm::frustum frustum = gfxm::make_frustum(_proj, _view);
        scn->getController<RenderController>()->getDrawList(dl, frustum);
        auto env = scn->find<RenderEnvironment>();
        if(env) { 
            renderer.setSkyGradient(env->getSkyGradient());
        }
        renderer.draw(&rvp, _proj, _view, dl);

        /*
        if(selected_objects) {
            for(auto n : selected_objects->getAll()) {
                std::list<Model*> list;
                n->getAttribsRecursive<Model>(list);
                for(auto m : list) {
                    m->addToDrawList(dl_silhouettes);
                }
            }
        }*/

        // TODO: REMOVE
        /*
        if(is_mouse_over && ImGui::IsMouseClicked(0) && !using_gizmo) {
            if(selected_objects) {
                renderer.drawPickPuffer(&fb_pick, _proj, _view, dl);
                uint32_t pix = 0;
                uint32_t err_pix = 0xFFFFFF;
                glReadPixels(mouse_pos.x, mouse_pos.y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, &pix);
                if(pix == err_pix) {
                    selected_objects->clear();
                } else if(pix < dl.solids.size()) {
                    ktNode* s = (ktNode*)dl.solids[pix].object_ptr;
                    while(s->getParent() != s->getRoot() && s->getParent() != 0) {
                        s = s->getParent();
                    }
                    selected_objects->clearAndAdd(s);
                    LOG("PICKED_SOLID: " << pix);
                } else {
                    ktNode* s = (ktNode*)dl.skins[pix - dl.solids.size()].object_ptr;
                    while(s->getParent() != s->getRoot() && s->getParent() != 0) {
                        s = s->getParent();
                    }
                    selected_objects->clearAndAdd(s);
                    LOG("PICKED_SKIN: " << pix - dl.solids.size());
                }
            }
        }*/

        // TODO: REMOVE
        /*
        renderer.drawSilhouettes(&fb_silhouette, _proj, _view, dl_silhouettes);
        blur(&fb_outline, fb_silhouette.getTextureId(0), gfxm::vec2(1, 0));
        blur(&fb_blur, fb_outline.getTextureId(0), gfxm::vec2(0, 1));
        blur(&fb_outline, fb_blur.getTextureId(0), gfxm::vec2(1, 0));
        blur(&fb_blur, fb_outline.getTextureId(0), gfxm::vec2(0, 1));
        cutout(&fb_outline, fb_blur.getTextureId(0), fb_silhouette.getTextureId(0));
        //drawOutline(&fb_outline, fb_silhouette.getTextureId(0));
        overlay(rvp.getFinalBuffer(), fb_outline.getTextureId(0));
        */
        rvp.getFinalBuffer()->bind();
        glViewport(0, 0, (GLsizei)vp_sz.x, (GLsizei)vp_sz.y);
        if(debug_draw_enabled) {
            dd.draw(_proj, _view);
        }
        dd.clear();

        GLuint buffers[] = {
            rvp.getFinalImage(),
            rvp.getGBuffer()->getAlbedoTexture(),
            rvp.getGBuffer()->getNormalTexture(),
            rvp.getGBuffer()->getRoughnessTexture(),
            rvp.getGBuffer()->getMetallicTexture(),
            rvp.getGBuffer()->getLightnessTexture(),
            rvp.getGBuffer()->getDepthTexture()
        };
        ImGui::GetWindowDrawList()->AddImage((void*)buffers[dbg_renderBufferId],
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
    }
    ImGui::EndChild();
}


bool GuiViewport::begin(gfxm::ivec2 sz) {
    if(ImGui::BeginChild(ImGui::GetID(this), ImVec2(sz.x, sz.y))) {
        ImGuizmo::SetDrawlist();

        dd.line(gfxm::vec3(-11.0f, .0f, -11.0f), gfxm::vec3(-10.0, .0f, -11.0f), gfxm::vec3(1.0f, .0f, .0f));
        dd.line(gfxm::vec3(-11.0f, .0f, -11.0f), gfxm::vec3(-11.0, 1.0f, -11.0f), gfxm::vec3(.0f, 1.0f, .0f));
        dd.line(gfxm::vec3(-11.0f, .0f, -11.0f), gfxm::vec3(-11.0, .0f, -10.0f), gfxm::vec3(.0f, .0f, 1.0f));
        dd.gridxz(
            gfxm::vec3(-20.0f, .0f, -20.0f),
            gfxm::vec3(20.0f, .0f, 20.0f),
            1,
            gfxm::vec3(0.04f, 0.04f, 0.04f)
        );

        auto window = ImGui::GetCurrentWindow();
        ImRect crect = window->ContentsRegionRect;
        ImGuizmo::SetRect(crect.Min.x, crect.Min.y, crect.Max.x - crect.Min.x, crect.Max.y - crect.Min.y);

        auto& io = ImGui::GetIO();
        is_mouse_over = ImGui::IsWindowHovered();
        window_in_focus = ImGui::IsRootWindowFocused();

        bb = window->ClipRect;
        ImVec2 cursor_pos = ImGui::GetCursorPos();
        pos.x = cursor_pos.x;
        pos.y = cursor_pos.y;
        vp_sz = ImVec2(bb.Max.x - bb.Min.x, bb.Max.y - bb.Min.y);

        if(viewport_sz.x != vp_sz.x || viewport_sz.y != vp_sz.y) {
            readback_buffer.resize(vp_sz.x * vp_sz.y * 4);
        }

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
        _proj = gfxm::perspective(gfxm::radian(45.0f), vp_sz.x/(float)vp_sz.y, 0.1f, 1000.0f);
        gfxm::transform tcam;
        cam_pos = gfxm::lerp(cam_pos, cam_pivot, 0.2f);
        cam_zoom_actual = gfxm::lerp(cam_zoom_actual, cam_zoom, 0.2f);
        tcam.position(cam_pos);
        cam_angle_x_actual = gfxm::lerp(cam_angle_x_actual, cam_angle_x, 0.5f);
        cam_angle_y_actual = gfxm::lerp(cam_angle_y_actual, cam_angle_y, 0.5f);
        tcam.rotate(cam_angle_y_actual, gfxm::vec3(0.0f, 1.0f, 0.0f));
        tcam.rotate(cam_angle_x_actual, tcam.right());
        tcam.translate(tcam.back() * cam_zoom_actual);
        _view = gfxm::inverse(tcam.matrix());
        
        gui_visible = true;
        return true;
    }
    gui_visible = false;
    return false;
}
void GuiViewport::end() {
    if(gui_visible) {
        rvp.getFinalBuffer()->bind();
        glViewport(0, 0, (GLsizei)vp_sz.x, (GLsizei)vp_sz.y);
        if(debug_draw_enabled) {
            dd.draw(_proj, _view);
        }
        dd.clear();

        GLuint buffers[] = {
            rvp.getFinalImage(),
            rvp.getGBuffer()->getAlbedoTexture(),
            rvp.getGBuffer()->getNormalTexture(),
            rvp.getGBuffer()->getRoughnessTexture(),
            rvp.getGBuffer()->getMetallicTexture(),
            rvp.getGBuffer()->getLightnessTexture(),
            rvp.getGBuffer()->getDepthTexture()
        };
        ImGui::GetWindowDrawList()->AddImage((void*)buffers[dbg_renderBufferId],
            bb.Min,
            bb.Max,
            ImVec2(0, 1),
            ImVec2(1, 0)
        );

        // Figure out 3d cursor pos
        float depth_pix = .0f;/*
        {
            auto mpos = this->getMousePos();

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, rvp.getGBuffer()->getDepthTexture());
            glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, readback_buffer.data());
            glBindTexture(GL_TEXTURE_2D, 0);
            int ret = glGetError();
            assert(ret == GL_NO_ERROR);

            if(mpos.x < 0 || mpos.y < 0 || mpos.x >= rvp.getGBuffer()->getWidth() || mpos.y >= rvp.getGBuffer()->getHeight()) {
                depth_pix = .0f;
            } else {
                depth_pix = readback_buffer[mpos.x + mpos.y * rvp.getGBuffer()->getWidth()];
            }

            glBindTexture(GL_TEXTURE_2D, rvp.getGBuffer()->getNormalTexture());
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, readback_buffer.data());
            glBindTexture(GL_TEXTURE_2D, 0);
            ret = glGetError();
            assert(ret == GL_NO_ERROR);

            cursor_xy_plane = gfxm::screenToWorldPlaneXY(gfxm::vec2(mpos.x, mpos.y), viewport_sz, _proj, _view);

            if(depth_pix == 1.0f) {
                cursor_3d_normal = gfxm::vec3(.0f, 1.0f, .0f);
                cursor_3d = cursor_xy_plane;
            } else {
                if (mpos.x < 0 || mpos.y < 0 || mpos.x >= rvp.getGBuffer()->getWidth() || mpos.y >= rvp.getGBuffer()->getHeight()) {
                    cursor_3d_normal = gfxm::vec3(.0f, 1.0f, .0f);
                }
                else {
                    memcpy(&cursor_3d_normal, &readback_buffer[(mpos.x + mpos.y * rvp.getGBuffer()->getWidth()) * 3], sizeof(float) * 3);
                    cursor_3d_normal = gfxm::normalize(cursor_3d_normal * 2.0 - gfxm::vec3(1,1,1));
                }

                depth_pix = depth_pix * 2.0 - 1.0;
                gfxm::vec3 position = 
                    gfxm::vec3((gfxm::vec2(mpos.x, mpos.y) / viewport_sz) * 2.0 - gfxm::vec2(1.0f, 1.0f), depth_pix);
                gfxm::mat4 inverse_view_projection = gfxm::inverse(_proj * _view);
                gfxm::vec4 wpos4 = inverse_view_projection * gfxm::vec4(position, 1.0);
                position = gfxm::vec3(wpos4.x, wpos4.y, wpos4.z) / wpos4.w;
                cursor_3d = position;
            }
        }*/

        ImGui::RenderText(bb.Min, MKSTR("GBuffer layer: " << dbg_renderBufferId).c_str());
        ImGui::RenderText(bb.Min + ImVec2(0, ImGui::GetTextLineHeight()), MKSTR("Pixel depth: " << depth_pix).c_str());
        ImGui::RenderText(
            bb.Min + ImVec2(0, ImGui::GetTextLineHeight() * 2), 
            MKSTR("3d cursor: [" << cursor_3d.x << ", " << cursor_3d.y << ", " << cursor_3d.z << "]").c_str()
        );
        dd.line(cursor_3d, cursor_3d + cursor_3d_normal, gfxm::vec3(1, 0, 0));
        dd.sphere(cursor_3d, .01f * this->cam_zoom_actual, gfxm::vec3(1,1,0));

        auto& io = ImGui::GetIO();
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
    }
    ImGui::EndChild();
}