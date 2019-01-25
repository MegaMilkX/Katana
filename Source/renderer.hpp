#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "scene.hpp"
#include "model.hpp"
#include "skin.hpp"
#include "light.hpp"
#include "transform.hpp"

#include "gl/frame_buffer.hpp"
#include "g_buffer.hpp"

#include "draw.hpp"

#include "shader_factory.hpp"

class Renderer : 
public ISceneProbe<Model>,
public ISceneProbe<Skin>,
public ISceneProbe<LightOmni> {
public:
    struct ObjectInfo {
        Model* mdl = 0;
        Skin* skin = 0;
    };

    Renderer()
    : scene(0) {
        texture_white_px.reset(new Texture2D());
        unsigned char wht[3] = { 255, 255, 255 };
        texture_white_px->Data(wht, 1, 1, 3);

        texture_black_px.reset(new Texture2D());
        unsigned char blk[3] = { 0, 0, 0};
        texture_black_px->Data(blk, 1, 1, 3);

        prog = ShaderFactory::getOrCreate(
            "solid",
            R"(#version 450
            in vec3 Position;
            in vec2 UV;
            in vec3 Normal;
            uniform mat4 mat_projection;
            uniform mat4 mat_view;
            uniform mat4 mat_model;
            out vec2 UVFrag;
            out vec3 NormalFrag;
            out vec3 ViewDir;
            void main()
            {
                ViewDir = (inverse(mat_view) * vec4(0, 0, -1, 0)).xyz;
                UVFrag = UV;
                NormalFrag = normalize((mat_model * vec4(Normal, 0.0)).xyz);
                gl_Position = mat_projection * mat_view * mat_model * vec4(Position, 1.0);
            })",
            R"(#version 450
            in vec2 UVFrag;
            in vec3 NormalFrag;
            in vec3 ViewDir;
            out vec4 out_albedo;
            uniform sampler2D tex_albedo;
            
            uniform vec3 u_color;

            void main()
            {
                vec3 albedo = u_color;
                float lightness = dot(NormalFrag, -ViewDir);
                vec3 n = ((NormalFrag) * 0.5f ) + 0.5f;
                out_albedo = texture(tex_albedo, UVFrag);
            })"
        );
        if(!prog) {
            LOG_ERR("Failed to get SOLID shader program");
        }

        prog_skin = ShaderFactory::getOrCreate(
            "skin",
            R"(#version 450
            #define MAX_BONE_COUNT 100
            in vec3 Position;
            in vec2 UV;
            in vec3 Normal;
            in vec4 BoneIndex4;
            in vec4 BoneWeight4;

            out vec3 fs_NormalModel;
            out vec3 ViewDir;

            uniform mat4 mat_projection;
            uniform mat4 mat_view;
            uniform mat4 mat_model;

            uniform mat4 inverseBindPose[MAX_BONE_COUNT];
            uniform mat4 bones[MAX_BONE_COUNT];

            void main() {
                ViewDir = (inverse(mat_view) * vec4(0, 0, -1, 0)).xyz;

                ivec4 bi = ivec4(
                    int(BoneIndex4.x), int(BoneIndex4.y),
                    int(BoneIndex4.z), int(BoneIndex4.w)
                );
                vec4 w = BoneWeight4;
                if(w.x + w.y + w.z + w.w > 1.0) {
                    w = normalize(w);
                }
                vec4 posModel = (
                    (bones[bi.x] * inverseBindPose[bi.x]) * vec4(Position, 1.0) * w.x +
                    (bones[bi.y] * inverseBindPose[bi.y]) * vec4(Position, 1.0) * w.y +
                    (bones[bi.z] * inverseBindPose[bi.z]) * vec4(Position, 1.0) * w.z +
                    (bones[bi.w] * inverseBindPose[bi.w]) * vec4(Position, 1.0) * w.w
                );
                vec3 normalSkinned = (
                    (bones[bi.x] * inverseBindPose[bi.x]) * vec4(Normal, 0.0) * w.x +
                    (bones[bi.y] * inverseBindPose[bi.y]) * vec4(Normal, 0.0) * w.y +
                    (bones[bi.z] * inverseBindPose[bi.z]) * vec4(Normal, 0.0) * w.z +
                    (bones[bi.w] * inverseBindPose[bi.w]) * vec4(Normal, 0.0) * w.w
                ).xyz;

                fs_NormalModel = normalize(vec4(normalSkinned, 0.0)).xyz;

                gl_Position = 
                    mat_projection *
                    mat_view *
                    posModel;
            })",
            R"(#version 450
            in vec2 UVFrag;
            in vec3 fs_NormalModel;
            in vec3 ViewDir;
            out vec4 out_albedo;
            uniform sampler2D tex_albedo;
            void main()
            {
                vec3 albedo = vec3(0.5, 0.5, 0.5);
                float lightness = dot(fs_NormalModel, -ViewDir);
                out_albedo = vec4(albedo * lightness, 1.0);
            })"
        );
        if(!prog_skin) {
            LOG_ERR("Failed to get SOLID shader program");
        }

        prog_pick = ShaderFactory::getOrCreate(
            "pick",
            R"(#version 450
            in vec3 Position;
            uniform mat4 mat_projection;
            uniform mat4 mat_view;
            uniform mat4 mat_model;
            void main()
            {
                gl_Position = mat_projection * mat_view * mat_model * vec4(Position, 1.0);
            })",
            R"(
            #version 450
            out vec4 out_albedo;
            uniform vec4 object_ptr;
            void main()
            {
                out_albedo = object_ptr;
            })"
        );
        if(!prog_pick) {
            LOG_ERR("Failed to get SOLID shader program");
        }
    }

    virtual void onCreateComponent(Model* mdl) {
        LOG("Model added");
        objects[mdl->getObject()].mdl = mdl;
    }
    virtual void onRemoveComponent(Model* mdl) {
        LOG("Model removed");
        objects.erase(mdl->getObject());
    }
    virtual void onCreateComponent(Skin* skin) {
        LOG("Skin added");
        objects[skin->getObject()].skin = skin;
    }
    virtual void onRemoveComponent(Skin* skin) {
        LOG("Skin removed");
        objects[skin->getObject()].skin = 0;
    }
    virtual void onCreateComponent(LightOmni* omni) {
        LOG("LightOmni added");
    }
    virtual void onRemoveComponent(LightOmni* omni) {
        LOG("LightOmni removed");
    }

    void setScene(Scene* scn) {
        if(scene) {
            scene->removeProbe<Model>();
            scene->removeProbe<Skin>();
            scene->removeProbe<LightOmni>();
        }
        scene = scn;
        if(scene) {
            scene->setProbe<Model>(this);
            scene->setProbe<Skin>(this);
            scene->setProbe<LightOmni>(this);
        }
    }

    void collectDrawLists(std::vector<gl::DrawInfo>& draw_list_solid, std::vector<gl::DrawInfo>& draw_list_skin) {
        for(auto kv : objects) {
            if(!kv.second.mdl) {
                continue;
            }
            if(!kv.second.mdl->mesh) {
                continue;
            }

            gl::DrawInfo draw_info = { 0 };
            draw_info.vao = kv.second.mdl->mesh->mesh.getVao();
            draw_info.index_count = kv.second.mdl->mesh->mesh.getIndexCount();
            draw_info.offset = 0;
            memcpy(draw_info.transform, (void*)&kv.second.mdl->getObject()->get<Transform>()->getTransform(), sizeof(draw_info.transform)); 
            
            draw_info.textures[0] = texture_white_px->GetGlName();
            draw_info.textures[1] = texture_black_px->GetGlName();
            draw_info.textures[2] = texture_black_px->GetGlName();
            draw_info.textures[3] = texture_white_px->GetGlName();

            if(kv.second.mdl->material) {
                Texture2D* tex = kv.second.mdl->material->albedo.get();
                if(tex) {
                    draw_info.textures[0] = tex->GetGlName();
                }
                tex = kv.second.mdl->material->normal.get();
                if(tex) {
                    draw_info.textures[1] = tex->GetGlName();
                }
                tex = kv.second.mdl->material->metallic.get();
                if(tex) {
                    draw_info.textures[2] = tex->GetGlName();
                }
                tex = kv.second.mdl->material->roughness.get();
                if(tex) {
                    draw_info.textures[3] = tex->GetGlName();
                }
            }

            if(kv.second.skin) {
                draw_info.user_ptr = (uint64_t)kv.second.skin;
                draw_list_skin.emplace_back(draw_info);
            } else {
                draw_list_solid.emplace_back(draw_info);
            }
        }
    }

    void drawSolidObjects(gl::ShaderProgram* prog, gfxm::mat4& projection, gfxm::mat4& view, std::vector<gl::DrawInfo>& draw_list) {
        glUseProgram(prog->getId());
        glUniformMatrix4fv(prog->getUniform("mat_projection"), 1, GL_FALSE, (float*)&projection);
        glUniformMatrix4fv(prog->getUniform("mat_view"), 1, GL_FALSE, (float*)&view);

        for(size_t i = 0; i < draw_list.size(); ++i) {
            gl::DrawInfo& d = draw_list[i];
            glUniformMatrix4fv(prog->getUniform("mat_model"), 1, GL_FALSE, d.transform);
            for(unsigned t = 0; t < sizeof(d.textures) / sizeof(d.textures[0]); ++t) {
                glActiveTexture(GL_TEXTURE0 + t);
                glBindTexture(GL_TEXTURE_2D, d.textures[t]);
            }

            glUniform3f(prog->getUniform("u_color"), 0.5f, 0.5f, 0.5f);

            glBindVertexArray(d.vao);
            glDrawElements(GL_TRIANGLES, d.index_count, GL_UNSIGNED_INT, (GLvoid*)d.offset);
        }
    }

    void drawSkinObjects(gl::ShaderProgram* prog, gfxm::mat4& projection, gfxm::mat4& view, std::vector<gl::DrawInfo>& draw_list) {
        prog->use();
        glUniformMatrix4fv(prog->getUniform("mat_projection"), 1, GL_FALSE, (float*)&projection);
        glUniformMatrix4fv(prog->getUniform("mat_view"), 1, GL_FALSE, (float*)&view);
        for(size_t i = 0; i < draw_list.size(); ++i) {
            gl::DrawInfo& d = draw_list[i];
            Skin* skin = (Skin*)d.user_ptr;

            std::vector<gfxm::mat4> inverse_bind_transforms;
            std::vector<gfxm::mat4> skin_transforms;
            for(auto a : skin->bones) {
                skin_transforms.emplace_back(scene->getComponent<Transform>(a)->getTransform());
            }
            for(auto a : skin->bind_pose) {
                inverse_bind_transforms.emplace_back(a);
            }

            GLuint loc = prog->getUniform("inverseBindPose[0]");
            glUniformMatrix4fv(
                loc,
                (std::min)((unsigned)100, (unsigned)inverse_bind_transforms.size()),
                GL_FALSE,
                (GLfloat*)inverse_bind_transforms.data()
            );
            loc = prog->getUniform("bones[0]");
            glUniformMatrix4fv(
                loc,
                (std::min)((unsigned)100, (unsigned)skin_transforms.size()),
                GL_FALSE,
                (GLfloat*)skin_transforms.data()
            );
            glBindVertexArray(d.vao);
            glDrawElements(GL_TRIANGLES, d.index_count, GL_UNSIGNED_INT, (GLvoid*)d.offset);
        }
    }

    void draw(GLuint framebuffer_index, GLsizei w, GLsizei h, gfxm::mat4 projection, gfxm::mat4 view) {
        std::vector<gl::DrawInfo> draw_list;
        std::vector<gl::DrawInfo> draw_list_skin;

        collectDrawLists(draw_list, draw_list_skin);

        glEnable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_index);
        glViewport(0, 0, w, h);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // TODO: Should only clear depth ?

        // =============================

        drawSolidObjects(prog, projection, view, draw_list);
        drawSkinObjects(prog_skin, projection, view, draw_list_skin);
    }

    void draw(gl::FrameBuffer* fb, gfxm::mat4 projection, gfxm::mat4 view) {
        draw(fb->getId(), fb->getWidth(), fb->getHeight(), projection, view);
    }

    void draw(GBuffer* g_buffer, gl::FrameBuffer* fb_final, gfxm::mat4& projection, gfxm::mat4& view) {

    }

    void drawPickBuffer(gl::FrameBuffer* fb, gfxm::mat4 projection, gfxm::mat4 view) {
        std::vector<gl::DrawInfo> draw_list;
        for(auto kv : objects) {
            if(!kv.second.mdl) continue;
            if(!kv.second.mdl->mesh) continue;
            
            gl::DrawInfo draw_info = { 0 };
            draw_info.vao = kv.second.mdl->mesh->mesh.getVao();
            draw_info.index_count = kv.second.mdl->mesh->mesh.getIndexCount();
            draw_info.offset = 0;
            memcpy(draw_info.transform, (void*)&kv.second.mdl->getObject()->get<Transform>()->getTransform(), sizeof(draw_info.transform)); 
            draw_info.user_ptr = (uint64_t)kv.second.mdl->getObject()->getId();
            draw_list.emplace_back(draw_info);
        }

        glEnable(GL_DEPTH_TEST);

        glBindFramebuffer(GL_FRAMEBUFFER, fb->getId());
        glViewport(0, 0, fb->getWidth(), fb->getHeight());
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // TODO: Should only clear depth

        glUseProgram(prog_pick->getId());
        glUniformMatrix4fv(prog_pick->getUniform("mat_projection"), 1, GL_FALSE, (float*)&projection);
        glUniformMatrix4fv(prog_pick->getUniform("mat_view"), 1, GL_FALSE, (float*)&view);

        for(size_t i = 0; i < draw_list.size(); ++i) {
            gl::DrawInfo& d = draw_list[i];
            glUniformMatrix4fv(prog_pick->getUniform("mat_model"), 1, GL_FALSE, d.transform);
            int r = (d.user_ptr & 0x000000FF) >>  0;
            int g = (d.user_ptr & 0x0000FF00) >>  8;
            int b = (d.user_ptr & 0x00FF0000) >> 16;  
            glUniform4f(prog_pick->getUniform("object_ptr"), r/255.0f, g/255.0f, b/255.0f, 1.0f);
            glBindVertexArray(d.vao);
            glDrawElements(GL_TRIANGLES, d.index_count, GL_UNSIGNED_INT, (GLvoid*)d.offset);
        }
    }
private:
    Scene* scene;

    gl::ShaderProgram* prog_pbr_solid;
    gl::ShaderProgram* prog_pbr_skin;

    gl::ShaderProgram* prog;
    gl::ShaderProgram* prog_skin;
    gl::ShaderProgram* prog_pick;
    std::shared_ptr<Texture2D> texture_white_px;
    std::shared_ptr<Texture2D> texture_black_px;

    std::map<SceneObject*, ObjectInfo> objects;
};

#endif
