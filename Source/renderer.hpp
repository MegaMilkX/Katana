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

#include "data_headers/icon_light_64.png.h"

inline void drawQuad()
{
    std::vector<float> vertices = {
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f
    };

    GLuint vao_handle = 0;
    GLuint vbuf;
    glGenBuffers(1, &vbuf);

    glGenVertexArrays(1, &vao_handle);
    glBindVertexArray(vao_handle);
    glBindBuffer(GL_ARRAY_BUFFER, vbuf);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, 
        sizeof(float) * 5, 0
    );
    glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE, 
        sizeof(float) * 5, (void*)(sizeof(float) * 3)
    );

    glBindBuffer(GL_ARRAY_BUFFER, vbuf);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), (void*)vertices.data(), GL_STREAM_DRAW);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDeleteVertexArrays(1, &vao_handle);
    glDeleteBuffers(1, &vbuf);
}

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

        texture_normal_px.reset(new Texture2D());
        unsigned char nrm[3] = { 128, 128, 255 };
        texture_normal_px->Data(nrm, 1, 1, 3);

        texture_def_roughness_px.reset(new Texture2D());
        unsigned char rgh[3] = { 230, 230, 230 };
        texture_def_roughness_px->Data(rgh, 1, 1, 3);

        auto make_icon_texture = [](const unsigned char* data, size_t sz, Texture2D& tex) {
            std::shared_ptr<DataSourceMemory> ds_mem(new DataSourceMemory((char*)data, (sz)));
            if(!tex.deserialize(ds_mem->open_stream(), sz)) {
                LOG_ERR("Failed to build file icon texture");
            }
            ds_mem->close_stream();
        };
        make_icon_texture(icon_light_64_png, sizeof(icon_light_64_png), tex_icon_light_64);

        prog_light_pass = ShaderFactory::getOrCreate(
            "light_pass",
            #include "shaders/v_quad.glsl"
            ,
            #include "shaders/f_light_pass.glsl"
        );

        prog_gbuf_solid = ShaderFactory::getOrCreate(
            "gbuf_solid_pbr",
            #include "shaders/v_solid_deferred_pbr.glsl"
            ,
            #include "shaders/f_deferred_pbr.glsl"
        );
        prog_gbuf_skin = ShaderFactory::getOrCreate(
            "gbuf_skin_pbr",
            #include "shaders/v_skin_deferred_pbr.glsl"
            ,
            #include "shaders/f_deferred_pbr.glsl"
        );

        prog_skybox = ShaderFactory::getOrCreate(
            "skybox",
            #include "shaders/v_skybox.glsl"
            ,
            #include "shaders/f_skybox.glsl"
        );

        prog_billboard_sprite = ShaderFactory::getOrCreate(
            "billboard_sprite",
            #include "shaders/v_billboard_sprite.glsl"
            ,
            #include "shaders/f_sprite.glsl"
        );
        prog_billboard_sprite_pick = ShaderFactory::getOrCreate(
            "billboard_sprite_pick",
            #include "shaders/v_billboard_sprite.glsl"
            ,
            #include "shaders/f_pick_alpha.glsl"
        );

        prog_pick = ShaderFactory::getOrCreate(
            "pick",
            #include "shaders/v_pick.glsl"
            ,
            #include "shaders/f_pick.glsl"
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
        lights_omni.insert(omni);
    }
    virtual void onRemoveComponent(LightOmni* omni) {
        LOG("LightOmni removed");
        lights_omni.erase(omni);
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
            draw_info.textures[1] = texture_normal_px->GetGlName();
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

            glUniform3f(prog->getUniform("u_tint"), 1.0f, 1.0f, 1.0f);

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

            for(unsigned t = 0; t < sizeof(d.textures) / sizeof(d.textures[0]); ++t) {
                glActiveTexture(GL_TEXTURE0 + t);
                glBindTexture(GL_TEXTURE_2D, d.textures[t]);
            }

            std::vector<gfxm::mat4> inverse_bind_transforms;
            std::vector<gfxm::mat4> skin_transforms;
            for(auto a : skin->bones) {
                skin_transforms.emplace_back(a->getTransform());
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
            glUniform3f(prog->getUniform("u_tint"), 1.0f, 1.0f, 1.0f);
            glBindVertexArray(d.vao);
            glDrawElements(GL_TRIANGLES, d.index_count, GL_UNSIGNED_INT, (GLvoid*)d.offset);
        }
    }

    void draw(
        GLuint framebuffer_index, 
        GLsizei w, GLsizei h, 
        gfxm::mat4& projection, 
        gfxm::mat4& view,
        gl::ShaderProgram* prog_solid,
        gl::ShaderProgram* prog_skin
    ) {
        std::vector<gl::DrawInfo> draw_list;
        std::vector<gl::DrawInfo> draw_list_skin;

        collectDrawLists(draw_list, draw_list_skin);

        glEnable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_index);
        glViewport(0, 0, w, h);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // TODO: Should only clear depth ?

        // =============================

        drawSolidObjects(prog_solid, projection, view, draw_list);
        drawSkinObjects(prog_skin, projection, view, draw_list_skin);
    }

    void drawGBuffer(GLuint fb, GLsizei w, GLsizei h, gfxm::mat4& proj, gfxm::mat4& view) {
        draw(fb, w, h, proj, view, prog_gbuf_solid, prog_gbuf_skin);
    }

    void draw(GBuffer* g_buffer, GLuint fb_final, GLsizei w, GLsizei h, gfxm::mat4 projection, gfxm::mat4 view) {
        drawGBuffer(
            g_buffer->getGlFramebuffer(),
            g_buffer->getWidth(),
            g_buffer->getHeight(),
            projection, view
        );

        glBindFramebuffer(GL_FRAMEBUFFER, fb_final);
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        prog_light_pass->use();

        const int MAX_OMNI_LIGHT = 10;
        gfxm::vec3 light_omni_pos[MAX_OMNI_LIGHT];
        gfxm::vec3 light_omni_col[MAX_OMNI_LIGHT];
        float light_omni_radius[MAX_OMNI_LIGHT];
        int i = 0;
        for(auto l : lights_omni) {
            light_omni_pos[i] = l->get<Transform>()->worldPosition();
            light_omni_col[i] = l->color * l->intensity;
            light_omni_radius[i] = l->radius;
            ++i;
            if(i >= MAX_OMNI_LIGHT) break;
        }

        glUniform3fv(prog_light_pass->getUniform("light_omni_pos"), i, (float*)light_omni_pos);
        glUniform3fv(prog_light_pass->getUniform("light_omni_col"), i, (float*)light_omni_col);
        glUniform1fv(prog_light_pass->getUniform("light_omni_radius"), i, (float*)light_omni_radius);
        glUniform1i(prog_light_pass->getUniform("light_omni_count"), i);

        gl::bindTexture2d(gl::TEXTURE_ALBEDO, g_buffer->getAlbedoTexture());
        gl::bindTexture2d(gl::TEXTURE_NORMAL, g_buffer->getNormalTexture());
        gl::bindTexture2d(gl::TEXTURE_METALLIC, g_buffer->getMetallicTexture());
        gl::bindTexture2d(gl::TEXTURE_ROUGHNESS, g_buffer->getRoughnessTexture());
        gl::bindCubeMap(gl::TEXTURE_ENVIRONMENT, scene->getSkybox().getIrradianceMap());
        gl::bindTexture2d(gl::TEXTURE_DEPTH, g_buffer->getDepthTexture());

        gfxm::vec4 view_pos4 = gfxm::inverse(view) * gfxm::vec4(0,0,0,1);
        glUniform3f(prog_light_pass->getUniform("view_pos"), view_pos4.x, view_pos4.y, view_pos4.z);
        glUniform2f(prog_light_pass->getUniform("viewport_size"), (float)w, (float)h);
        gfxm::mat4 inverse_view_projection =
            gfxm::inverse(projection * view);
        glUniformMatrix4fv(prog_light_pass->getUniform("inverse_view_projection"), 1, GL_FALSE, (float*)&inverse_view_projection);
        drawQuad();

        glDepthFunc(GL_LEQUAL);
        prog_skybox->use();
        glUniformMatrix4fv(prog_skybox->getUniform("mat_projection"), 1, GL_FALSE, (float*)&projection);
        glUniformMatrix4fv(prog_skybox->getUniform("mat_view"), 1, GL_FALSE, (float*)&view);
        gl::bindCubeMap(gl::TEXTURE_ENVIRONMENT, scene->getSkybox().getSkyMap());
        drawCube();

        prog_billboard_sprite->use();
        glUniformMatrix4fv(prog_billboard_sprite->getUniform("mat_projection"), 1, GL_FALSE, (float*)&projection);
        glUniformMatrix4fv(prog_billboard_sprite->getUniform("mat_view"), 1, GL_FALSE, (float*)&view);
        glUniform2f(prog_billboard_sprite->getUniform("viewport_size"), (float)w, (float)h);
        glUniform2f(prog_billboard_sprite->getUniform("sprite_size"), (float)64, (float)64);
        gl::bindTexture2d(gl::TEXTURE_DIFFUSE, tex_icon_light_64.GetGlName());

        for(auto l : lights_omni) {
            glUniformMatrix4fv(prog_billboard_sprite->getUniform("mat_model"), 1, GL_FALSE, (float*)&l->get<Transform>()->getTransform());
            drawQuad();
        }
    }

    void draw(GBuffer* g_buffer, gl::FrameBuffer* fb_final, gfxm::mat4& projection, gfxm::mat4& view) {
        draw(
            g_buffer, 
            fb_final->getId(), fb_final->getWidth(), fb_final->getHeight(),
            projection, view
        );
    }

    void drawPickBuffer(gl::FrameBuffer* fb, gfxm::mat4 projection, gfxm::mat4 view) {
        int w = fb->getWidth();
        int h = fb->getHeight();

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

        prog_billboard_sprite_pick->use();
        glUniformMatrix4fv(prog_billboard_sprite_pick->getUniform("mat_projection"), 1, GL_FALSE, (float*)&projection);
        glUniformMatrix4fv(prog_billboard_sprite_pick->getUniform("mat_view"), 1, GL_FALSE, (float*)&view);
        glUniform2f(prog_billboard_sprite_pick->getUniform("viewport_size"), (float)w, (float)h);
        glUniform2f(prog_billboard_sprite_pick->getUniform("sprite_size"), (float)64, (float)64);
        gl::bindTexture2d(gl::TEXTURE_DIFFUSE, tex_icon_light_64.GetGlName());
        for(auto l : lights_omni) {
            glUniformMatrix4fv(prog_billboard_sprite_pick->getUniform("mat_model"), 1, GL_FALSE, (float*)&l->get<Transform>()->getTransform());
            int r = (l->getObject()->getId() & 0x000000FF) >>  0;
            int g = (l->getObject()->getId() & 0x0000FF00) >>  8;
            int b = (l->getObject()->getId() & 0x00FF0000) >> 16;  
            glUniform4f(prog_billboard_sprite_pick->getUniform("object_ptr"), r/255.0f, g/255.0f, b/255.0f, 1.0f);
            drawQuad();
        }
    }
private:
    Scene* scene;

    gl::ShaderProgram* prog_gbuf_solid;
    gl::ShaderProgram* prog_gbuf_skin;
    gl::ShaderProgram* prog_light_pass;
    gl::ShaderProgram* prog_skybox;
    gl::ShaderProgram* prog_billboard_sprite;
    gl::ShaderProgram* prog_billboard_sprite_pick;

    gl::ShaderProgram* prog_pick;
    std::shared_ptr<Texture2D> texture_white_px;
    std::shared_ptr<Texture2D> texture_black_px;
    std::shared_ptr<Texture2D> texture_normal_px;
    std::shared_ptr<Texture2D> texture_def_roughness_px;

    std::map<SceneObject*, ObjectInfo> objects;
    std::set<LightOmni*> lights_omni;

    Texture2D tex_icon_light_64;
};

#endif
