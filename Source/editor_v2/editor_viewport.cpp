#include "editor_viewport.hpp"

#include "../common/lib/imgui_wrap.hpp"
#include "../common/lib/imgui/imgui_internal.h"
#include "../common/lib/imguizmo/ImGuizmo.h"

#include "../common/resource/resource_factory.h"
#include "../common/resource/texture2d.h"

#include "../common/util/has_suffix.hpp"

#include "scene_object_from_fbx.hpp"

#include "editor.hpp"

bool mouse_look = false;
bool mouse_look_alt = false;
float cam_angle_y = gfxm::radian(45.0f);
float cam_angle_x = gfxm::radian(-25.0f);
float cam_zoom = 5.0f;
gfxm::vec3 cam_pivot;

gfxm::vec3 cam_pos;
float cam_zoom_actual = 0.0f;

enum GIZMO_MODE {
    GIZMO_TRANSLATE,
    GIZMO_ROTATE,
    GIZMO_SCALE
};
static GIZMO_MODE gizmo_mode = GIZMO_TRANSLATE;

EditorViewport::EditorViewport() {
    vp.init(640, 480);
    dd.init();

    input().getTable().addActionKey("MouseLeft", "MOUSE_LEFT");
    input().getTable().addActionKey("MouseRight", "MOUSE_RIGHT");
    input().getTable().addActionKey("MouseMiddle", "MOUSE_MIDDLE");
    input().getTable().addActionKey("MouseLookAlt", "KB_LEFT_ALT");
    input().getTable().addActionKey("ResetEditorCam", "KB_Z");
    input().getTable().addActionKey("GizmoT", "KB_W");
    input().getTable().addActionKey("GizmoR", "KB_E");
    input().getTable().addActionKey("GizmoS", "KB_R");
    input().getTable().addAxisKey("MoveCamX", "MOUSE_X", 1.0);
    input().getTable().addAxisKey("MoveCamY", "MOUSE_Y", 1.0);
    input().getTable().addAxisKey("CameraZoom", "MOUSE_SCROLL", 1.0);

    input_lis = input().createListener();
    input_lis->bindActionPress("MouseMiddle", [this](){
        mouse_look = true;
    });
    input_lis->bindActionRelease("MouseMiddle", [this](){
        mouse_look = false;
    });
    input_lis->bindActionPress("MouseLookAlt", [this](){ mouse_look_alt = true; });
    input_lis->bindActionRelease("MouseLookAlt", [this](){ mouse_look_alt = false; });
    input_lis->bindAxis("MoveCamX", [this](float v){
        if(editorState().is_play_mode) return;
        if(mouse_look && mouse_look_alt){
            cam_angle_y += (-v * 0.01f);
        } else if(mouse_look){
            gfxm::transform tcam;
            tcam.rotate(cam_angle_y, gfxm::vec3(0.0f, 1.0f, 0.0f));
            tcam.rotate(cam_angle_x, tcam.right());
            float mod = cam_zoom;
            gfxm::vec3 right = tcam.right();
            cam_pivot += right * -v * 0.01f * mod * 0.15f;
        }
    });
    input_lis->bindAxis("MoveCamY", [this](float v){
        if(editorState().is_play_mode) return;
        if(mouse_look && mouse_look_alt){
            cam_angle_x += (-v * 0.01f);
        } else if(mouse_look){
            gfxm::transform tcam;
            tcam.rotate(cam_angle_y, gfxm::vec3(0.0f, 1.0f, 0.0f));
            tcam.rotate(cam_angle_x, tcam.right());
            float mod = cam_zoom;
            gfxm::vec3 up = tcam.up();
            cam_pivot += up * v * 0.01f * mod * 0.15f;
        }
    });
    input_lis->bindAxis("CameraZoom", [this](float v){
        if(editorState().is_play_mode) return;
        float mod = cam_zoom;
        cam_zoom += -v * mod * 0.15f;
    });
    input_lis->bindActionPress("ResetEditorCam", [this](){
        if(editorState().is_play_mode) return;
        //cam_angle_y = gfxm::radian(45.0f);
        //cam_angle_x = gfxm::radian(-25.0f);
        if(editor->getSelectedObject()) {
            auto o = editor->getSelectedObject();
            cam_zoom = gfxm::length(
                o->getAabb().to - o->getAabb().from
            ) * 0.8f + 0.01f;
            cam_pivot = gfxm::lerp(
                o->getAabb().from,
                o->getAabb().to,
                0.5f
            );
        } else {
            cam_zoom = 5.0f;
            cam_pivot = gfxm::vec3(.0f, .0f, .0f);
        }
    });
    input_lis->bindActionPress("GizmoT", [this](){
        if(editorState().is_play_mode) return;
        gizmo_mode = GIZMO_TRANSLATE;
    });
    input_lis->bindActionPress("GizmoR", [this](){
        if(editorState().is_play_mode) return;
        gizmo_mode = GIZMO_ROTATE;
    });
    input_lis->bindActionPress("GizmoS", [this](){
        if(editorState().is_play_mode) return;
        gizmo_mode = GIZMO_SCALE;
    });
}
EditorViewport::~EditorViewport() {
    dd.cleanup();
    input().removeListener(input_lis);
}

void EditorViewport::init(Editor* editor, GameScene* scene) {
    this->editor = editor;
    gfx_mgr.setScene(scene);
    anim_mgr.setScene(scene);
}

void EditorViewport::update(Editor* editor) {
    editor->getScene()->update();
    dd.clear();

    dd.line(
        gfxm::vec3(.0f, .0f, .0f),
        gfxm::vec3(.0, 1.0f, .0f),
        gfxm::vec3(1.0f, .0f, .0f)
    );
    dd.gridxz(
        gfxm::vec3(-10.0f, .0f, -10.0f),
        gfxm::vec3(10.0f, .0f, 10.0f),
        1,
        gfxm::vec3(0.2f, 0.2f, 0.2f)
    );
    for(size_t i = 0; i < editor->getScene()->objectCount(); ++i) {
        auto o = editor->getScene()->getObject(i);
        dd.aabb(o->getAabb(), gfxm::vec3(1.0f, 0.7f, 0.2f));
    }
    if(editor->getSelectedObject()) {
        dd.aabb(
            editor->getSelectedObject()->getAabb(),
            gfxm::vec3(1.0f, 1.0f, 1.0f)
        );
    }
    /*
    dd.grid3d(
        gfxm::vec3(-5.0f, -5.0f, -5.0f),
        gfxm::vec3(5.0f, 5.0f, 5.0f),
        2.0f,
        gfxm::vec3(0.5f, 0.5f, 0.5f)
    );*/

    if(ImGui::Begin("Scene")) {
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
            
            if(gizmo_mode == GIZMO_TRANSLATE) {
                ImGuizmo::Manipulate((float*)&view, (float*)&proj, ImGuizmo::OPERATION::TRANSLATE, ImGuizmo::MODE::LOCAL, (float*)&model, (float*)&dModel, 0);
                if(ImGuizmo::IsUsing()){
                    gfxm::vec4 dT = dModel[3];
                    editor->getSelectedObject()->getTransform()->translate(dT);
                    editor->getEditorScene().getObjectDesc(editor->getSelectedObject())->updateData();
                    editor->getSelectedObject()->getRoot()->refreshAabb();
                }
            } else if(gizmo_mode == GIZMO_ROTATE) {
                ImGuizmo::Manipulate((float*)&view, (float*)&proj, ImGuizmo::OPERATION::ROTATE, ImGuizmo::MODE::LOCAL, (float*)&model, (float*)&dModel, 0);
                if(ImGuizmo::IsUsing()){
                    editor->getSelectedObject()->getTransform()->setTransform(model);
                    editor->getEditorScene().getObjectDesc(editor->getSelectedObject())->updateData();
                    editor->getSelectedObject()->getRoot()->refreshAabb();
                }
            } else if(gizmo_mode == GIZMO_SCALE) {
                ImGuizmo::Manipulate((float*)&view, (float*)&proj, ImGuizmo::OPERATION::SCALE, ImGuizmo::MODE::LOCAL, (float*)&model, (float*)&dModel, 0);
                if(ImGuizmo::IsUsing()){
                    editor->getSelectedObject()->getTransform()->setTransform(model);
                    editor->getEditorScene().getObjectDesc(editor->getSelectedObject())->updateData();
                    editor->getSelectedObject()->getRoot()->refreshAabb();
                }
            }
        }

        anim_mgr.update(1.0f/60.0f);
        
        DrawList dl;
        gfx_mgr.getDrawList(dl);
        renderer.draw(&vp, proj, view, dl);

        vp.getFinalBuffer()->bind();
        glViewport(0, 0, (GLsizei)sz.x, (GLsizei)sz.y);
        dd.draw(proj, view);
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
                    GameObject* new_so = editor->getScene()->create<GameObject>();
                    objectFromFbx(fname, new_so);
                    //new_so->get<Transform>()->position(wpt_xy);
                    editor->setSelectedObject(new_so);
                }
            }
            ImGui::EndDragDropTarget();
        }
        ImGui::End();
    }
}