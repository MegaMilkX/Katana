#ifndef EDITOR_VIEWPORT_HPP
#define EDITOR_VIEWPORT_HPP

#include <cctype>

#include "editor_window.hpp"

#include "../common/g_buffer.hpp"

#include "../common/gl/frame_buffer.hpp"
#include "../common/gl/vertex_buffer.hpp"
#include "../common/gl/shader_program.h"

#include "../common/gl/indexed_mesh.hpp"

#include "../common/gfxm.hpp"

#include "../common/draw.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "../common/renderer.hpp"

#include <GLFW/glfw3.h>

#include "../common/input/input_mgr.hpp"

#include "editor_state.hpp"

#include "../common/lib/imguizmo/ImGuizmo.h"

#include "../common/util/has_suffix.hpp"
#include "../common/util/filesystem.hpp"

#include "../common/scene_from_fbx.hpp"

#include "../common/debug_draw.hpp"

class EditorViewport : public EditorWindow {
public:
    enum DISPLAY_BUFFER {
        ALBEDO,
        POSITION,
        NORMAL,
        METALLIC,
        ROUGHNESS,
        FINAL
    };

    EditorViewport(Renderer* renderer)
    : renderer(renderer) {
        frame_buffer.pushBuffer(GL_RGB, GL_UNSIGNED_BYTE);
        fb_silhouette.pushBuffer(GL_RED, GL_UNSIGNED_BYTE);
        fb_fin.pushBuffer(GL_RGB, GL_UNSIGNED_BYTE);
        fb_pick.pushBuffer(GL_RGB, GL_UNSIGNED_BYTE);

        float vertices[] = {
            -1.0f, -1.0f,
            1.0f, -1.0f,
            1.0f, 1.0f,
            -1.0f, 1.0f,
        };
        float uvs[] = {
            0.0f, 0.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,
            0.0f, 1.0f
        };
        uint32_t indices[] = {
            0, 1, 2, 0, 2, 3
        };

        //vb.data(vertices, sizeof(float) * 12);
        //ib.data(indices, sizeof(uint32_t) * 6);

        quad_mesh.setAttribData(gl::POSITION, vertices, sizeof(float) * 8, 2, GL_FLOAT, GL_FALSE);
        quad_mesh.setAttribData(gl::UV, uvs, sizeof(float) * 8, 2, GL_FLOAT, GL_FALSE);
        quad_mesh.setIndices(indices, 6);

        prog_silhouette = ShaderFactory::getOrCreate(
            "viewport_silhouette",
            R"(#version 450
            in vec3 Position;
            uniform mat4 mat_projection;
            uniform mat4 mat_view;
            uniform mat4 mat_model;
            void main()
            {
                gl_Position = mat_projection * mat_view * mat_model * vec4(Position, 1.0);
            })",
            R"(#version 450
            out vec4 out_albedo;
            void main()
            {
                out_albedo = vec4(1.0, 1.0, 1.0, 1.0);
            })"
        );

        prog_fin = ShaderFactory::getOrCreate(
            "viewport_fin",
            R"(#version 450
            in vec2 Position;
            in vec2 UV;
            out vec2 UVFrag;
            void main()
            {
                UVFrag = UV;
                gl_Position = vec4(Position.xy, 0.0, 1.0);
            })",
            R"(#version 450
            in vec2 UVFrag;
            out vec4 out_albedo;
            uniform sampler2D tex_0;
            uniform sampler2D tex_1;

            vec4 blur5(sampler2D image, vec2 uv, vec2 resolution, vec2 direction) {
                vec4 color = vec4(0.0);
                vec2 off1 = vec2(1.3333333333333333) * direction;
                color += texture2D(image, uv) * 0.29411764705882354;
                color += texture2D(image, uv + (off1 / resolution)) * 0.35294117647058826;
                color += texture2D(image, uv - (off1 / resolution)) * 0.35294117647058826;
                return color; 
            }
            vec4 blur13(sampler2D image, vec2 uv, vec2 resolution, vec2 direction) {
                vec4 color = vec4(0.0);
                vec2 off1 = vec2(1.411764705882353) * direction;
                vec2 off2 = vec2(3.2941176470588234) * direction;
                vec2 off3 = vec2(5.176470588235294) * direction;
                color += texture2D(image, uv) * 0.1964825501511404;
                color += texture2D(image, uv + (off1 / resolution)) * 0.2969069646728344;
                color += texture2D(image, uv - (off1 / resolution)) * 0.2969069646728344;
                color += texture2D(image, uv + (off2 / resolution)) * 0.09447039785044732;
                color += texture2D(image, uv - (off2 / resolution)) * 0.09447039785044732;
                color += texture2D(image, uv + (off3 / resolution)) * 0.010381362401148057;
                color += texture2D(image, uv - (off3 / resolution)) * 0.010381362401148057;
                return color;
            }

            void main()
            {
                vec4 blurred = blur13(tex_1, UVFrag, vec2(256, 256), vec2(1, 0));
                blurred += blur13(tex_1, UVFrag, vec2(256, 256), vec2(0, 1));
                blurred = clamp(blurred, vec4(0.0), vec4(1.0));
                vec4 border = (vec4(blurred.xxx, 1.0) - vec4(texture(tex_1, UVFrag).xxx, 1.0));
                border *= 2.0f;
                border = clamp(border, vec4(0.0), vec4(1.0));
                out_albedo = 
                    texture(tex_0, UVFrag) + 
                    border;
            })"
        );

        input_lis = input().createListener();

        input_lis->bindActionPress(
            "MouseLookHold",
            [this](){
                mouse_look = true;
            }
        );
        input_lis->bindActionRelease(
            "MouseLookHold",
            [this](){
                mouse_look = false;
            }
        );

        input_lis->bindActionPress(
            "MouseLookAlt",
            [this](){ mouse_look_alt = true; }
        );
        input_lis->bindActionRelease(
            "MouseLookAlt",
            [this](){ mouse_look_alt = false; }
        );

        input_lis->bindAxis(
            "MoveCamX",
            [this](float v){
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
            }
        );
        input_lis->bindAxis(
            "MoveCamY",
            [this](float v){
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
            }
        );

        input_lis->bindAxis(
            "CameraZoom",
            [this](float v){
                float mod = cam_zoom;
                cam_zoom += -v * mod * 0.15f;
            }
        );

        input_lis->bindActionPress(
            "ResetEditorCam",
            [this](){
                gfxm::vec3 resetPos(0.0f, 0.0f, 0.0f);
                float zoom = 1.0f;
                if(editorState().selected_object) {
                    std::vector<Model*> models;
                    editorState().selected_object->findAllRecursive(models);
                    gfxm::vec3 median_point;
                    float radius = 1.0f;
                    bool first = true;

                    median_point = editorState().selected_object->get<Transform>()->getTransform() * gfxm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

                    for(auto m : models) {
                        for(size_t i = 0; i < m->segmentCount(); ++i) {
                            if(!m->getSegment(i).mesh) continue;
                            gfxm::mat4 world_transform = m->getObject()->get<Transform>()->getTransform();
                            gfxm::vec3 mpt = gfxm::lerp(m->getSegment(i).mesh->aabb.from, m->getSegment(i).mesh->aabb.to, 0.5f);
                            mpt = world_transform * gfxm::vec4(mpt.x, mpt.y, mpt.z, 1.0f);
                            if(!first)
                                median_point = gfxm::lerp(median_point, mpt, 0.5f);
                            else 
                                median_point = mpt;
                            first = false;

                            gfxm::vec3 diag = m->getSegment(i).mesh->aabb.to - m->getSegment(i).mesh->aabb.from;
                            diag = world_transform * gfxm::vec4(diag.x, diag.y, diag.z, 0.0f);
                            float r = gfxm::length(diag);
                            if(radius < r) radius = r;
                        }
                    }
                    resetPos = median_point;
                    zoom = radius;
                } 
                cam_pivot = resetPos;
                cam_zoom = zoom;
            }
        );

        input_lis->bindActionPress("View0", [this](){ disp_buf = FINAL; });
        input_lis->bindActionPress("View1", [this](){ disp_buf = ALBEDO; });
        input_lis->bindActionPress("View2", [this](){ disp_buf = POSITION; });
        input_lis->bindActionPress("View3", [this](){ disp_buf = NORMAL; });
        input_lis->bindActionPress("View4", [this](){ disp_buf = METALLIC; });
        input_lis->bindActionPress("View5", [this](){ disp_buf = ROUGHNESS; });
    }
    bool mouse_look = false;
    bool mouse_look_alt = false;
    float cam_angle_y = gfxm::radian(45.0f);
    float cam_angle_x = gfxm::radian(-25.0f);
    float cam_zoom = 5.0f;
    gfxm::vec3 cam_pivot;

    gfxm::vec3 cam_pos;
    float cam_zoom_actual = 0.0f;

    ~EditorViewport() {
        input().removeListener(input_lis);
    }

    std::string Name() {
        return "Viewport";
    }
    void Update() {
        if(ImGui::BeginMenuBar()) {
            if(ImGui::MenuItem("Reset view")) {}
            if(ImGui::BeginMenu("Camera")) {
                if(ImGui::MenuItem("Perspective")) {}
                if(ImGui::MenuItem("Orthogonal")) {}
                if(ImGui::MenuItem("Top")) {}
                if(ImGui::MenuItem("Bottom")) {}
                if(ImGui::MenuItem("Left")) {}
                if(ImGui::MenuItem("Right")) {}
                if(ImGui::MenuItem("Front")) {}
                if(ImGui::MenuItem("Back")) {}
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        ImGuiIO& io = ImGui::GetIO();

        ImVec2 sz = ImGui::GetWindowSize();
        if(vp_size.x != sz.x || vp_size.y != sz.y) {
            vp_size = sz;
            
            g_buffer.resize((unsigned)sz.x, (unsigned)sz.y);

            frame_buffer.reinitBuffers((unsigned)sz.x, (unsigned)sz.y);
            fb_silhouette.reinitBuffers((unsigned)sz.x, (unsigned)sz.y);
            fb_fin.reinitBuffers((unsigned)sz.x, (unsigned)sz.y);
            fb_pick.reinitBuffers((unsigned)sz.x, (unsigned)sz.y);
        }

        if(ImGui::IsWindowFocused()) {
            // TODO: ?
        }

        gfxm::mat4 proj = gfxm::perspective(gfxm::radian(45.0f), sz.x/(float)sz.y, 0.01f, 1000.0f);
        gfxm::transform tcam;
        cam_pos = gfxm::lerp(cam_pos, cam_pivot, 0.2f);
        cam_zoom_actual = gfxm::lerp(cam_zoom_actual, cam_zoom, 0.2f);
        tcam.position(cam_pos);
        tcam.rotate(cam_angle_y, gfxm::vec3(0.0f, 1.0f, 0.0f));
        tcam.rotate(cam_angle_x, tcam.right());
        tcam.translate(tcam.back() * cam_zoom_actual);
        gfxm::mat4 view = gfxm::inverse(tcam.matrix()); 

        renderer->draw(&g_buffer, &frame_buffer, proj, view);
        renderer->drawEditorHelpers(frame_buffer.getId(), frame_buffer.getWidth(), frame_buffer.getHeight(), proj, view);
        DebugDraw::getInstance()->draw(proj, view);

        if(editorState().selected_object) {
            std::vector<Model*> models;
            std::vector<gl::DrawInfo> draw_list;
            editorState().selected_object->findAllRecursive(models);
            for(auto m : models) {
                for(size_t i = 0; i < m->segmentCount(); ++i) {
                    Model::Segment& seg = m->getSegment(i);
                    gl::DrawInfo di = { 0 };
                    di.index_count = seg.mesh->mesh.getIndexCount();
                    di.offset = 0;
                    memcpy(di.transform, (float*)&m->getObject()->get<Transform>()->getTransform(), sizeof(di.transform));
                    di.vao = seg.mesh->mesh.getVao();

                    draw_list.emplace_back(di);
                }
            }

            gl::draw(
                fb_silhouette.getId(), 
                prog_silhouette->getId(), 
                sz.x, sz.y,
                prog_silhouette->getUniform("mat_projection"),
                (float*)&proj,
                prog_silhouette->getUniform("mat_view"),
                (float*)&view,
                prog_silhouette->getUniform("mat_model"),
                draw_list.data(),
                draw_list.size()
            );
        } else {
            fb_silhouette.bind();
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }

        fb_fin.bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        prog_fin->use();
        glActiveTexture(GL_TEXTURE0);
        // NOTE: Color buffer for this viewport
        GLuint color_buf = 0;
        switch(disp_buf) {
        case ALBEDO: color_buf = g_buffer.getAlbedoTexture(); break;
        case POSITION: color_buf = g_buffer.getDepthTexture(); break;
        case NORMAL: color_buf = g_buffer.getNormalTexture(); break;
        case METALLIC: color_buf = g_buffer.getMetallicTexture(); break;
        case ROUGHNESS: color_buf = g_buffer.getRoughnessTexture(); break;
        case FINAL: color_buf = frame_buffer.getTextureId(0); break;
        }
        glBindTexture(GL_TEXTURE_2D, color_buf);
        glActiveTexture(GL_TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_2D, fb_silhouette.getTextureId(0));
        quad_mesh.bind();
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        //ImGui::Text(MKSTR("Cursor: " << lcl_cursor.x << ", " << lcl_cursor.y).c_str());

        ImVec2 imguiWindowCorner = ImGui::GetCursorScreenPos();
        lcl_cursor = gfxm::ivec2(
            cursor_pos.x - (int)imguiWindowCorner.x,
            sz.y - (cursor_pos.y - (int)imguiWindowCorner.y)
        );
        
        renderer->drawPickBuffer(vp_size.x, vp_size.y, proj, view);

        ImGuizmo::SetRect((float)imguiWindowCorner.x, (float)imguiWindowCorner.y, (float)sz.x, (float)sz.y);

        bool using_gizmo = false;
        if(editorState().selected_object && editorState().selected_object->find<Transform>()) {
            gfxm::mat4 model = editorState().selected_object->get<Transform>()->getTransform();
            gfxm::mat4 dModel(1.0f);
            
            ImGuizmo::Manipulate((float*)&view, (float*)&proj, ImGuizmo::OPERATION::TRANSLATE, ImGuizmo::MODE::LOCAL, (float*)&model, (float*)&dModel, 0);
            if(ImGuizmo::IsUsing()){
                using_gizmo = true;
                gfxm::vec4 dT = dModel[3];
                editorState().selected_object->get<Transform>()->translate(dT);
                // TODO: Transform changed
            }
        }

        Renderer::PickInfo pick_info = { 0 };
        if(ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_AllowWhenOverlapped) &&
            (lcl_cursor.x >= 0 && lcl_cursor.y >= 0)
        ) {
            pick_info = renderer->pick(lcl_cursor.x, lcl_cursor.y);
        }

        if(io.MouseClicked[0] && !using_gizmo &&
            ImGui::IsWindowHovered() &&
            (lcl_cursor.x >= 0 && lcl_cursor.y >= 0)) 
        {
            if(pick_info.object && (editorState().selected_object != pick_info.object->getTopObject())) {
                editorState().selected_object = pick_info.object->getTopObject();
            } else {
                editorState().selected_object = pick_info.object;
            }
        }

        GLuint color_tex = fb_fin.getTextureId(0);//frame_buffer.getTextureId(0);
        //auto tex = getResource<Texture2D>("image.png");

        ImVec2 imvp_size = ImVec2(
            ImGui::GetCursorScreenPos().x + sz.x, 
            ImGui::GetCursorScreenPos().y + sz.y
        );
        ImGui::GetWindowDrawList()->AddImage((void*)color_tex, 
            ImVec2(ImGui::GetCursorScreenPos()),
            imvp_size, 
            ImVec2(0, 1), 
            ImVec2(1, 0)
        );

        unsigned char pixel[3] = { 0, 0, 0 };
        if(ImGui::IsWindowHovered() &&
        (lcl_cursor.x >= 0 && lcl_cursor.y >= 0)) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, frame_buffer.getId());
            glReadBuffer(GL_COLOR_ATTACHMENT0);
            
            glReadPixels(lcl_cursor.x, lcl_cursor.y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, (void*)&pixel);
            glReadBuffer(GL_NONE);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        }
        ImGui::Text(
            MKSTR("Pixel: (" << (int)pixel[0] << ", " << (int)pixel[1] << ", " << (int)pixel[2] << ")").c_str()
        );

        ImGui::Dummy(sz);

        gfxm::vec3 wpt_xy = gfxm::screenToWorldPlaneXY(
            gfxm::vec2(lcl_cursor.x, lcl_cursor.y),
            gfxm::vec2(sz.x, sz.y),
            proj, view
        );

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_ASSET_FILE")) {
                //SceneObject* tgt_dnd_so = *(SceneObject**)payload->Data;
                //tgt_dnd_so->setParent(so);
                std::string fname = (char*)payload->Data;
                LOG("Payload received: " << fname);

                for(size_t i = 0; i < fname.size(); ++i) {
                    fname[i] = (std::tolower(fname[i]));
                }
                if(pick_info.object) {
                    if(has_suffix(fname, ".mat")) {
                        Model* mdl = pick_info.object->find<Model>();
                        if(mdl) {
                            mdl->getSegment(pick_info.mesh_segment).material = getResource<Material>(fname);
                        }
                    } else if(has_suffix(fname, ".jpg") ||
                        has_suffix(fname, ".jpeg") ||
                        has_suffix(fname, ".png") ||
                        has_suffix(fname, ".tga") ||
                        has_suffix(fname, ".bmp") ||
                        has_suffix(fname, ".psd") ||
                        has_suffix(fname, ".gif") ||
                        has_suffix(fname, ".hdr") ||
                        has_suffix(fname, ".pic")
                    ) {
                        Material material;
                        material.albedo = getResource<Texture2D>(fname);
                        file_stream strm(fname + ".mat");
                        material.serialize(strm);

                        LOG(fname + ".mat");
                        GlobalDataRegistry().Add(
                            fname + ".mat",
                            DataSourceRef(new DataSourceFilesystem(get_module_dir() + "\\" + fname + ".mat"))
                        );

                        Model* mdl = pick_info.object->find<Model>();
                        if(mdl) {
                            mdl->getSegment(pick_info.mesh_segment).material = getResource<Material>(fname + ".mat");
                        }
                    }
                }
                if(has_suffix(fname, ".fbx")) {
                    SceneObject* new_so = scene->createObject();
                    sceneFromFbx(
                        fname, 
                        scene, 
                        new_so
                    );
                    new_so->get<Transform>()->position(wpt_xy);
                    editorState().selected_object = new_so;
                }
            }
            ImGui::EndDragDropTarget();
        }
        
        if (ImGui::BeginPopupContextWindow()) {
            if(ImGui::BeginMenu("Create...")) {
                if(ImGui::MenuItem("LightOmni")) {
                    auto so = scene->createObject();
                    so->setName("LightOmni");
                    so->get<LightOmni>();
                    so->get<Transform>()->position(context_menu_wpt_xy);
                    editorState().selected_object = so;
                }
            }
            ImGui::EndPopup();
        } else {
            context_menu_wpt_xy = wpt_xy;
        }
    }
    gfxm::vec3 context_menu_wpt_xy;

    void setScene(Scene* scn) {
        scene = scn;
    }
    void setCursorPos(gfxm::ivec2 cursor) {
        cursor_pos = cursor;
    }
private:
    InputListener* input_lis;

    gfxm::ivec2 cursor_pos;
    gfxm::ivec2 lcl_cursor;

    DISPLAY_BUFFER disp_buf = FINAL;

    Renderer* renderer;
    Scene* scene = 0;

    std::vector<gl::DrawInfo> draw_list;
    
    gl::IndexedMesh quad_mesh;
    GBuffer g_buffer;
    gl::FrameBuffer frame_buffer;
    gl::FrameBuffer fb_silhouette;
    gl::FrameBuffer fb_fin;
    gl::FrameBuffer fb_pick;
    gl::ShaderProgram* prog_silhouette;
    gl::ShaderProgram* prog_fin;
    ImVec2 vp_size = ImVec2(0, 0);
};

#endif
