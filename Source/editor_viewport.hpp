#ifndef EDITOR_VIEWPORT_HPP
#define EDITOR_VIEWPORT_HPP

#include "editor_window.hpp"

#include "g_buffer.hpp"

#include "gl/frame_buffer.hpp"
#include "gl/vertex_buffer.hpp"
#include "gl/shader_program.h"

#include "gl/indexed_mesh.hpp"

#include "gfxm.hpp"

#include "draw.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "renderer.hpp"

#include <GLFW/glfw3.h>

#include "input/input_mgr.hpp"

#include "editor_state.hpp"

#include "lib/imguizmo/ImGuizmo.h"

class EditorViewport : public EditorWindow {
public:
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

        {
            gl::Shader vs(GL_VERTEX_SHADER);
            gl::Shader fs(GL_FRAGMENT_SHADER);
            vs.source(R"(
                #version 450
                in vec3 Position;
                uniform mat4 mat_projection;
                uniform mat4 mat_view;
                uniform mat4 mat_model;
                void main()
                {
                    gl_Position = mat_projection * mat_view * mat_model * vec4(Position, 1.0);
                })"
            );
            fs.source(R"(
                #version 450
                out vec4 out_albedo;
                void main()
                {
                    out_albedo = vec4(1.0, 1.0, 1.0, 1.0);
                })"
            );
            vs.compile();
            fs.compile();

            prog_silhouette.attachShader(&vs);
            prog_silhouette.attachShader(&fs);
            prog_silhouette.bindAttrib(gl::POSITION, "Position");
            prog_silhouette.bindFragData(0, "out_albedo");
            prog_silhouette.link();
            prog_silhouette.use();
        }
        //
        {
            gl::Shader vs(GL_VERTEX_SHADER);
            gl::Shader fs(GL_FRAGMENT_SHADER);
            vs.source(R"(
                #version 450
                in vec2 Position;
                in vec2 UV;
                out vec2 UVFrag;
                void main()
                {
                    UVFrag = UV;
                    gl_Position = vec4(Position.xy, 0.0, 1.0);
                })"
            );
            fs.source(R"(
                #version 450
                in vec2 UVFrag;
                out vec4 out_albedo;
                uniform sampler2D tex0;
                uniform sampler2D tex1;

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
                    vec4 blurred = blur13(tex1, UVFrag, vec2(256, 256), vec2(1, 0));
                    blurred += blur13(tex1, UVFrag, vec2(256, 256), vec2(0, 1));
                    blurred = clamp(blurred, vec4(0.0), vec4(1.0));
                    vec4 border = (vec4(blurred.xxx, 1.0) - vec4(texture(tex1, UVFrag).xxx, 1.0));
                    border *= 2.0f;
                    border = clamp(border, vec4(0.0), vec4(1.0));
                    out_albedo = 
                        texture(tex0, UVFrag) + 
                        border;
                })"
            );
            vs.compile();
            fs.compile();

            prog_fin.attachShader(&vs);
            prog_fin.attachShader(&fs);
            prog_fin.bindAttrib(gl::POSITION, "Position");
            prog_fin.bindAttrib(gl::UV, "UV");
            prog_fin.bindFragData(0, "out_albedo");
            prog_fin.link();
            prog_fin.use();
            glUniform1i(prog_fin.getUniform("tex0"), 0);
            glUniform1i(prog_fin.getUniform("tex1"), 1);
        }

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
                if(editorState().selected_object) {
                    resetPos = editorState().selected_object->get<Transform>()->worldPosition();
                }
                cam_pivot = resetPos;
            }
        );
    }
    bool mouse_look = false;
    bool mouse_look_alt = false;
    float cam_angle_y = 0.0f;
    float cam_angle_x = 0.0f;
    float cam_zoom = 5.0f;
    gfxm::vec3 cam_pivot;

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

        gfxm::mat4 proj = gfxm::perspective(0.90f, sz.x/(float)sz.y, 0.01f, 1000.0f);
        gfxm::transform tcam;
        tcam.position(cam_pivot);
        tcam.rotate(cam_angle_y, gfxm::vec3(0.0f, 1.0f, 0.0f));
        tcam.rotate(cam_angle_x, tcam.right());
        tcam.translate(tcam.back() * cam_zoom);
        gfxm::mat4 view = gfxm::inverse(tcam.matrix()); 
        
        renderer->draw(
            &frame_buffer,
            proj,
            view
        );
        //renderer->draw(&g_buffer, &frame_buffer, proj, view);

        if(editorState().selected_object) {
            gl::DrawInfo di = { 0 };
            Model* mdl = editorState().selected_object->find<Model>();
            if(mdl && mdl->mesh) {
                di.index_count = mdl->mesh->mesh.getIndexCount();
                di.offset = 0;
                memcpy(di.transform, (float*)&editorState().selected_object->get<Transform>()->getTransform(), sizeof(di.transform));
                di.vao = mdl->mesh->mesh.getVao();

                gl::draw(
                    fb_silhouette.getId(), 
                    prog_silhouette.getId(), 
                    sz.x, sz.y,
                    prog_silhouette.getUniform("mat_projection"),
                    (float*)&proj,
                    prog_silhouette.getUniform("mat_view"),
                    (float*)&view,
                    prog_silhouette.getUniform("mat_model"),
                    &di,
                    1
                );
            } else {
                gl::draw(
                    fb_silhouette.getId(), 
                    prog_silhouette.getId(), 
                    sz.x, sz.y,
                    prog_silhouette.getUniform("mat_projection"),
                    (float*)&proj,
                    prog_silhouette.getUniform("mat_view"),
                    (float*)&view,
                    prog_silhouette.getUniform("mat_model"),
                    0, 0
                );
            }
        } else {
            gl::draw(
                fb_silhouette.getId(), 
                prog_silhouette.getId(), 
                sz.x, sz.y,
                prog_silhouette.getUniform("mat_projection"),
                (float*)&proj,
                prog_silhouette.getUniform("mat_view"),
                (float*)&view,
                prog_silhouette.getUniform("mat_model"),
                0, 0
            );
        }

        fb_fin.bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        prog_fin.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, frame_buffer.getTextureId(0));
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
        
        renderer->drawPickBuffer(
            &fb_pick,
            proj,
            view
        );

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
            }
        }

        if(ImGui::IsWindowHovered() && 
            io.MouseClicked[0] && 
            (lcl_cursor.x >= 0 && lcl_cursor.y >= 0) &&
            !using_gizmo
        ) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, fb_pick.getId());
            glReadBuffer(GL_COLOR_ATTACHMENT0);
            char pixel[3];
            glReadPixels(lcl_cursor.x, lcl_cursor.y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, (void*)&pixel);
            glReadBuffer(GL_NONE);

            int picked_index = pixel[0] + pixel[1] * 256 + pixel[2] * 256 * 256;
            if(picked_index != -65793) {
                if(this->scene) {
                    if(picked_index < this->scene->objectCount()) {
                        editorState().selected_object = this->scene->getObject(picked_index);
                    }
                }
            } else {
                editorState().selected_object = 0;
            }
            glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        }

        GLuint color_tex = fb_fin.getTextureId(0);//frame_buffer.getTextureId(0);
        //auto tex = getResource<Texture2D>("image.png");

        ImGui::GetWindowDrawList()->AddImage((void*)color_tex, 
            ImVec2(ImGui::GetCursorScreenPos()),
            ImVec2(
                ImGui::GetCursorScreenPos().x + sz.x, 
                ImGui::GetCursorScreenPos().y + sz.y
            ), 
            ImVec2(0, 1), 
            ImVec2(1, 0)
        );        
    }

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

    Renderer* renderer;
    Scene* scene = 0;

    std::vector<gl::DrawInfo> draw_list;
    
    gl::IndexedMesh quad_mesh;
    GBuffer g_buffer;
    gl::FrameBuffer frame_buffer;
    gl::FrameBuffer fb_silhouette;
    gl::FrameBuffer fb_fin;
    gl::FrameBuffer fb_pick;
    gl::ShaderProgram prog_silhouette;
    gl::ShaderProgram prog_fin;
    gl::ShaderProgram prog_pick;
    ImVec2 vp_size = ImVec2(0, 0);
};

#endif
