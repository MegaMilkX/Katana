#include "editor_viewport.hpp"

#include "../common/lib/imgui_wrap.hpp"
#include "../common/lib/imgui/imgui_internal.h"
#include "../common/lib/imguizmo/ImGuizmo.h"

#include "../common/resource/resource_tree.hpp"
#include "../common/resource/texture2d.h"

#include "../common/util/has_suffix.hpp"

#include "../common/util/scene_object_from_fbx.hpp"

#include "editor.hpp"

#include "../common/scene/controllers/render_controller.hpp"

#include "../common/platform/platform.hpp"

bool mouse_look = false;
bool mouse_look_alt = false;
float cam_angle_y = gfxm::radian(45.0f);
float cam_angle_x = gfxm::radian(-25.0f);
float cam_zoom = 5.0f;
gfxm::vec3 cam_pivot;

gfxm::vec3 cam_pos;
float cam_zoom_actual = 0.0f;

EditorViewport::EditorViewport() {
    vp.init(640, 480);
    dd.init();

    input().getTable().addActionKey("MouseLeft", "MOUSE_LEFT");
    input().getTable().addActionKey("MouseRight", "MOUSE_RIGHT");
    input().getTable().addActionKey("MouseMiddle", "MOUSE_MIDDLE");
    input().getTable().addActionKey("MouseLookAlt", "KB_LEFT_ALT");

    input_lis = input().createListener();
    input_lis->bindActionPress("MouseMiddle", [this](){
        //if(!mouse_is_over_vp) return;
        mouse_look = true;
    });
    input_lis->bindActionRelease("MouseMiddle", [this](){
        mouse_look = false;
    });
    input_lis->bindActionPress("MouseLookAlt", [this](){ gvp.camMode(GuiViewport::CAM_ORBIT); mouse_look_alt = true; });
    input_lis->bindActionRelease("MouseLookAlt", [this](){ gvp.camMode(GuiViewport::CAM_PAN);mouse_look_alt = false; });
    input_lis->bindAxis("MoveCamX", [this](float v){
        //gvp.camMove(gfxm::vec2(-v, .0f));
        /*
        if(mouse_look && mouse_look_alt){
            gvp.camRotate(gfxm::vec2(.0f, -v * 0.01f));
        } else if(mouse_look){
            gvp.camMove(gfxm::vec2(-v * 0.01f, .0f));
        }*/
    });
    input_lis->bindAxis("MoveCamY", [this](float v){
        //gvp.camMove(gfxm::vec2(.0f, -v));
        /*
        if(mouse_look && mouse_look_alt){
            gvp.camRotate(gfxm::vec2(-v * 0.01f, .0f));
        } else if(mouse_look){
            gvp.camMove(gfxm::vec2(.0f, v * 0.01f));
        }*/
    });
    input_lis->bindAxis("CameraZoom", [this](float v){
        //if(!mouse_is_over_vp) return;
        //float mod = cam_zoom;
        //cam_zoom += -v * mod * 0.15f;
    });

    oct_o = oct.createObject();
}
EditorViewport::~EditorViewport() {
    dd.cleanup();
    input().removeListener(input_lis);
}

void EditorViewport::init(Editor* editor, GameScene* scene) {
    this->editor = editor;
}

void EditorViewport::update(Editor* editor) {
    /*if(ImGui::Begin("Scene")) {
        mouse_is_over_vp = ImGui::IsWindowHovered();
        window_in_focus = ImGui::IsRootWindowFocused();

        ktNode* selected_object = 0;
        if(!editor->getSelectedObjects().empty()) {
            selected_object = *editor->getSelectedObjects().getAll().begin();
        }
        gvp.draw(editor->getScene(), selected_object, gfxm::ivec2(0, 0));
        
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_ASSET_FILE")) {
                std::string fname = (char*)payload->Data;
                LOG("Payload received: " << fname);
                for(size_t i = 0; i < fname.size(); ++i) {
                    fname[i] = (std::tolower(fname[i]));
                }
                if(has_suffix(fname, ".fbx")) {
                    //ktNode* new_so = editor->getScene()->getRoot()->createChild();
                    //objectFromFbx(fname, new_so);
                    //new_so->get<Transform>()->position(wpt_xy);
                    //editor->setSelectedObject(new_so);
                    //editor->backupScene("import object from fbx");
                } else if(has_suffix(fname, ".so")) {
                    //editor->getScene()->getRoot()->createChild()->read(get_module_dir() + "/" + platformGetConfig().data_dir + "/" + fname);
                }
            }
            ImGui::EndDragDropTarget();
        }
        ImGui::End();
    }
 */
    return;
/*
    editor->getScene()->update();
    dd.clear();
    if(editor->getState().debug_draw) {
        editor->getScene()->debugDraw(dd);
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
    
    auto& selected = editor->getSelectedObjects();
    for(auto& o : selected.getAll()) {
        dd.aabb(
            o->getAabb(),
            gfxm::vec3(1.0f, 1.0f, 1.0f)
        );
    }

    auto speco = editor->getScene()->getRoot()->findObject("sp");
    if(speco) {
        gfxm::vec3 pt = speco->getTransform()->getWorldPosition();
        oct_o->pos = pt;
        oct.fit(oct_o);
        dd.point(pt, gfxm::vec3(1,0,0));
        dd.aabb(gfxm::aabb(oct_o->box.from + oct_o->pos, oct_o->box.from + oct_o->pos), gfxm::vec3(1,0,0));
        
    }
    oct.debugDraw(&dd);

    if(ImGui::Begin("Scene")) {
        mouse_is_over_vp = ImGui::IsWindowHovered();
        window_in_focus = ImGui::IsRootWindowFocused();

        ImVec2 sz = ImGui::GetWindowSize();
        ImVec2 pad = ImVec2(
            ImGui::GetWindowPos().x - ImGui::GetCursorScreenPos().x,
            ImGui::GetWindowPos().y - ImGui::GetCursorScreenPos().y
        );
        sz = ImVec2(sz.x + pad.x, sz.y + pad.y);
        ImVec2 imvp_size = ImVec2(
            ImGui::GetCursorScreenPos().x + sz.x,
            ImGui::GetCursorScreenPos().y + sz.y
        );

        vp.resize(sz.x, sz.y);

        gfxm::mat4 proj = gfxm::perspective(gfxm::radian(45.0f), sz.x/(float)sz.y, 0.01f, 1000.0f);
        gfxm::transform tcam;
        cam_pos = gfxm::lerp(cam_pos, cam_pivot, 0.2f);
        cam_zoom_actual = gfxm::lerp(cam_zoom_actual, cam_zoom, 0.2f);
        tcam.position(cam_pos);
        tcam.rotate(cam_angle_y, gfxm::vec3(0.0f, 1.0f, 0.0f));
        tcam.rotate(cam_angle_x, tcam.right());
        tcam.translate(tcam.back() * cam_zoom_actual);
        gfxm::mat4 view = gfxm::inverse(tcam.matrix());

        
        if(editor->getSelectedObject()) {
            ImGuizmo::SetRect(
                ImVec2(ImGui::GetCursorScreenPos()).x, 
                ImVec2(ImGui::GetCursorScreenPos()).y, 
                sz.x, 
                sz.y
            );
            gfxm::mat4 model = editor->getSelectedObject()->getTransform()->getWorldTransform();
            gfxm::mat4 dModel(1.0f);

            ImGuizmo::MODE space_mode = ImGuizmo::MODE::LOCAL;
            if(editor->getState().tgizmo_space == TGIZMO_WORLD) {
                space_mode = ImGuizmo::MODE::WORLD;
            }
            
            if(editor->getState().tgizmo_mode == TGIZMO_T) {
                ImGuizmo::Manipulate((float*)&view, (float*)&proj, ImGuizmo::OPERATION::TRANSLATE, space_mode, (float*)&model, (float*)&dModel, 0);
                if(ImGuizmo::IsUsing()){
                    gfxm::vec4 dT = dModel[3];
                    editor->getSelectedObject()->getTransform()->translate(
                        gfxm::inverse(editor->getSelectedObject()->getTransform()->getParentTransform()) * dT
                    );
                    editor->getSelectedObject()->getRoot()->refreshAabb();
                }
            } else if(editor->getState().tgizmo_mode == TGIZMO_R) {
                ImGuizmo::Manipulate((float*)&view, (float*)&proj, ImGuizmo::OPERATION::ROTATE, space_mode, (float*)&model, (float*)&dModel, 0);
                if(ImGuizmo::IsUsing()){
                    gfxm::quat q = gfxm::to_quat(gfxm::to_mat3(dModel));
                    editor->getSelectedObject()->getTransform()->rotate(q);
                    editor->getSelectedObject()->getRoot()->refreshAabb();
                }
            } else if(editor->getState().tgizmo_mode == TGIZMO_S) {
                ImGuizmo::Manipulate((float*)&view, (float*)&proj, ImGuizmo::OPERATION::SCALE, space_mode, (float*)&model, (float*)&dModel, 0);
                if(ImGuizmo::IsUsing()){
                    gfxm::mat4 m4 = model;
                    m4 = gfxm::inverse(editor->getSelectedObject()->getTransform()->getParentTransform()) * m4;
                    gfxm::vec3 dscl(
                        gfxm::length(m4[0]),
                        gfxm::length(m4[1]),
                        gfxm::length(m4[2])
                    );

                    editor->getSelectedObject()->getTransform()->setScale(dscl);
                    editor->getSelectedObject()->getRoot()->refreshAabb();
                }
            }
        } 
        
        DrawList dl;
        editor->getScene()->getController<RenderController>()->getDrawList(dl);
        renderer.draw(&vp, proj, view, dl);

        vp.getFinalBuffer()->bind();
        glViewport(0, 0, (GLsizei)sz.x, (GLsizei)sz.y);
        
        if(editor->getState().debug_draw) {
            dd.draw(proj, view);
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);        

        ImGui::GetWindowDrawList()->AddImage((void*)vp.getFinalImage(), 
            ImVec2(ImGui::GetCursorScreenPos()),
            imvp_size, 
            ImVec2(0, 1), 
            ImVec2(1, 0)
        );

        ImGui::Dummy(ImVec2(sz.x - 10, sz.y - 10));
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_ASSET_FILE")) {
                std::string fname = (char*)payload->Data;
                LOG("Payload received: " << fname);
                for(size_t i = 0; i < fname.size(); ++i) {
                    fname[i] = (std::tolower(fname[i]));
                }
                if(has_suffix(fname, ".fbx")) {
                    ktNode* new_so = editor->getScene()->getRoot()->createChild();
                    objectFromFbx(fname, new_so);
                    //new_so->get<Transform>()->position(wpt_xy);
                    editor->setSelectedObject(new_so);
                    editor->backupScene("import object from fbx");
                } else if(has_suffix(fname, ".so")) {
                    editor->getScene()->getRoot()->read(fname);
                }
            }
            ImGui::EndDragDropTarget();
        }
        ImGui::End();
    }*/
}

void EditorViewport::recenterCamera() {
    //cam_angle_y = gfxm::radian(45.0f);
    //cam_angle_x = gfxm::radian(-25.0f);
/*
    auto& selected = editor->getSelectedObjects();
    if(selected.empty()) {
        gvp.resetCamera(
            gfxm::vec3(.0f, .0f, .0f),
            5.0f
        );
        return;
    }
    ktNode* o = *selected.getAll().begin();

    gvp.resetCamera(
        gfxm::lerp(o->getAabb().from, o->getAabb().to, 0.5f),
        gfxm::length(o->getAabb().to - o->getAabb().from) * 0.8f + 0.01f
    ); */
}

GuiViewport& EditorViewport::getViewport() {
    return gvp;
}