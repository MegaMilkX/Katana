#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "render_viewport.hpp"
#include "gfxm.hpp"
#include "gl/indexed_mesh.hpp"
#include "shader_factory.hpp"
#include "resource/texture2d.h"
#include "resource/cube_map.hpp"

#include "resource/resource_factory.h"

#include "draw_list.hpp"

#include "gl/uniform_buffers.hpp"

#include "util/animation/curve.h"

#include "data_headers/ibl_brdf_lut.png.h"

#define SKIN_BONE_LIMIT 256

void drawQuad();

struct uCommon3d {
    gfxm::mat4 view;
    gfxm::mat4 projection;
};
struct uBones {
    gfxm::mat4 pose[SKIN_BONE_LIMIT];
};

class Renderer {
public:
    Renderer() {
        {
            curve<gfxm::vec3> curv;
            curv[0.0f] = gfxm::vec3(0 / 255.0f, 0 / 255.0f, 0 / 255.0f);
            curv[.5f] = gfxm::vec3(3 / 255.0f, 3 / 255.0f, 5 / 255.0f);
            curv[.55f] = gfxm::vec3(255 / 255.0f, 74 / 255.0f, 5 / 255.0f);
            curv[.60f] = gfxm::vec3(160 / 255.0f, 137 / 255.0f, 129 / 255.0f);
            curv[.7f] = gfxm::vec3(1 / 255.0f, 44 / 255.0f, 95 / 255.0f);
            curv[1.0f] = gfxm::vec3(1 / 255.0f, 11 / 255.0f, 36 / 255.0f);
            char color[300];
            for(int i = 0; i < 300; i += 3) {
                float y = i / 300.0f;
                gfxm::vec3 c = curv.at(y);
                color[i] = c.x * 255;
                color[i + 1] = c.y * 255;
                color[i + 2] = c.z * 255;
            }
            //env_map = retrieve<CubeMap>("env.hdr");
            env_map.reset(new CubeMap());
            env_map->data(color, 1, 100, 3);

            irradiance_map.reset(new CubeMap());
            irradiance_map->makeIrradianceMap(env_map);
        }

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
        
        tex_ibl_brdf_lut.reset(new Texture2D());
        dstream dstrm;
        dstrm.setBuffer(std::vector<char>(ibl_brdf_lut_png, ibl_brdf_lut_png + sizeof(ibl_brdf_lut_png)));
        tex_ibl_brdf_lut->deserialize(dstrm, sizeof(ibl_brdf_lut_png));

        prog_light_pass = ShaderFactory::getOrCreate(
            "light_pass",
            #include "../common/shaders/v_quad.glsl"
            ,
            #include "../common/shaders/f_light_pass.glsl"
        );
        prog_gbuf_solid = ShaderFactory::getOrCreate(
            "gbuf_solid_pbr",
            #include "../common/shaders/v_solid_deferred_pbr.glsl"
            ,
            #include "../common/shaders/f_deferred_pbr.glsl"
        );
        prog_gbuf_skin = ShaderFactory::getOrCreate(
            "gbuf_skin_pbr",
            #include "../common/shaders/v_skin_deferred_pbr.glsl"
            ,
            #include "../common/shaders/f_deferred_pbr.glsl"
        );
        prog_skybox = ShaderFactory::getOrCreate(
            "skybox",
            #include "../common/shaders/v_skybox.glsl"
            ,
            #include "../common/shaders/f_skybox.glsl"
        );
    }

    gl::UniformBuffer<uCommon3d, 0> ubufCommon3d;
    gl::UniformBuffer<uBones, 1> ubufBones;

    void beginFrame(RenderViewport* vp, gfxm::mat4& proj, gfxm::mat4& view) {
        this->proj = proj;
        this->view = view;
        g_buffer = vp->getGBuffer();
        w = (float)vp->getWidth();
        h = (float)vp->getHeight();
        glEnable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_FRAMEBUFFER, vp->getGBuffer()->getGlFramebuffer());
        glViewport(0, 0, (GLsizei)vp->getWidth(), (GLsizei)vp->getHeight());

        ubufCommon3d.upload(uCommon3d{view, proj});

        ubufCommon3d.bind();
        ubufBones.bind();

        glClear(GL_DEPTH_BUFFER_BIT);
    }

    void endFrame() {
        // ==== Skybox ========================
        glDepthFunc(GL_LEQUAL);
        prog_skybox->use();
        glUniformMatrix4fv(prog_skybox->getUniform("mat_projection"), 1, GL_FALSE, (float*)&proj);
        glUniformMatrix4fv(prog_skybox->getUniform("mat_view"), 1, GL_FALSE, (float*)&view);
        gl::bindCubeMap(gl::TEXTURE_ENVIRONMENT, env_map->getId());
        drawCube();

        glUseProgram(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void drawSolidObjects(const DrawList& draw_list) {
        auto prog = prog_gbuf_solid;

        prog->use();

        for(size_t i = 0; i < draw_list.solidCount(); ++i) {
            auto o = draw_list.getSolid(i);
            
            GLuint textures[4] = {
                texture_white_px->GetGlName(),
                texture_normal_px->GetGlName(),
                texture_black_px->GetGlName(),
                texture_white_px->GetGlName()
            };
            gfxm::vec3 tint(1,1,1);
            if(o->material) {
                if(o->material->albedo) textures[0] = o->material->albedo->GetGlName();
                if(o->material->normal) textures[1] = o->material->normal->GetGlName();
                if(o->material->metallic) textures[2] = o->material->metallic->GetGlName();
                if(o->material->roughness) textures[3] = o->material->roughness->GetGlName();
                tint = o->material->tint;
            }
            glActiveTexture(GL_TEXTURE0 + 0);
            glBindTexture(GL_TEXTURE_2D, textures[0]);
            glActiveTexture(GL_TEXTURE0 + 1);
            glBindTexture(GL_TEXTURE_2D, textures[1]);
            glActiveTexture(GL_TEXTURE0 + 2);
            glBindTexture(GL_TEXTURE_2D, textures[2]);
            glActiveTexture(GL_TEXTURE0 + 3);
            glBindTexture(GL_TEXTURE_2D, textures[3]);
            glUniform3f(prog->getUniform("u_tint"), tint.x, tint.y, tint.z);

            glBindVertexArray(o->vao);
            glUniformMatrix4fv(prog->getUniform("mat_model"), 1, GL_FALSE, (float*)&o->transform);    
            glDrawElements(GL_TRIANGLES, o->indexCount, GL_UNSIGNED_INT, 0);
        }
    }

    void drawSkinObjects(const DrawList& dl) {
        auto p = prog_gbuf_skin;
        p->use();

        for(size_t i = 0; i < dl.skinCount(); ++i) {
            auto o = dl.getSkin(i);

            GLuint textures[4] = {
                texture_white_px->GetGlName(),
                texture_normal_px->GetGlName(),
                texture_black_px->GetGlName(),
                texture_white_px->GetGlName()
            };
            gfxm::vec3 tint(1,1,1);
            if(o->material) {
                if(o->material->albedo) textures[0] = o->material->albedo->GetGlName();
                if(o->material->normal) textures[1] = o->material->normal->GetGlName();
                if(o->material->metallic) textures[2] = o->material->metallic->GetGlName();
                if(o->material->roughness) textures[3] = o->material->roughness->GetGlName();
                tint = o->material->tint;
            }
            glActiveTexture(GL_TEXTURE0 + 0);
            glBindTexture(GL_TEXTURE_2D, textures[0]);
            glActiveTexture(GL_TEXTURE0 + 1);
            glBindTexture(GL_TEXTURE_2D, textures[1]);
            glActiveTexture(GL_TEXTURE0 + 2);
            glBindTexture(GL_TEXTURE_2D, textures[2]);
            glActiveTexture(GL_TEXTURE0 + 3);
            glBindTexture(GL_TEXTURE_2D, textures[3]);
            glUniform3f(p->getUniform("u_tint"), tint.x, tint.y, tint.z);

            gfxm::mat4 bone_m[SKIN_BONE_LIMIT];
            unsigned bone_count = (std::min)((unsigned)SKIN_BONE_LIMIT, (unsigned)o->bind_transforms.size());
            for(unsigned i = 0; i < bone_count; ++i) {
                bone_m[i] = o->bone_transforms[i] * o->bind_transforms[i];
            }

            uBones bones;
            memcpy(bones.pose, bone_m, sizeof(gfxm::mat4) * bone_count);
            ubufBones.upload(bones);

            glBindVertexArray(o->vao);
            glUniformMatrix4fv(p->getUniform("mat_model"), 1, GL_FALSE, (float*)&o->transform);    
            glDrawElements(GL_TRIANGLES, o->indexCount, GL_UNSIGNED_INT, 0);
        }
    }

    void draw(RenderViewport* vp, gfxm::mat4& proj, gfxm::mat4& view, const DrawList& draw_list, bool draw_final_on_screen = false) {
        beginFrame(vp, proj, view);
        
        drawSolidObjects(draw_list);
        drawSkinObjects(draw_list);

        // ==== Light pass ===============
        if(draw_final_on_screen) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        } else {
            glBindFramebuffer(GL_FRAMEBUFFER, vp->getFinalBuffer()->getId());
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        prog_light_pass->use();
        
        const int MAX_OMNI_LIGHT = 10;
        gfxm::vec3 light_omni_pos[MAX_OMNI_LIGHT];
        gfxm::vec3 light_omni_col[MAX_OMNI_LIGHT];
        float light_omni_radius[MAX_OMNI_LIGHT];/*
        int i = 0;
        for(auto l : lights_omni) {
            light_omni_pos[i] = l->get<Transform>()->worldPosition();
            light_omni_col[i] = l->color * l->intensity;
            light_omni_radius[i] = l->radius;
            ++i;
            if(i >= MAX_OMNI_LIGHT) break;
        }*/
        //light_omni_pos[0] = gfxm::vec3(0.0f, 0.0f, 0.5f);
        //light_omni_pos[0] = gfxm::vec3(1.0f, 1.0f, 1.0f);
        //light_omni_radius[0] = 50.0f;
        {
            int count = std::min((int)draw_list.omniLightCount(), MAX_OMNI_LIGHT);
            for(size_t i = 0; i < count && i < MAX_OMNI_LIGHT; ++i) {
                auto& l = draw_list.getOmniLight(i);
                light_omni_pos[i] = l.translation;
                light_omni_col[i] = l.color * l.intensity;
                light_omni_radius[i] = l.radius;
            }
            glUniform3fv(prog_light_pass->getUniform("light_omni_pos"), count, (float*)light_omni_pos);
            glUniform3fv(prog_light_pass->getUniform("light_omni_col"), count, (float*)light_omni_col);
            glUniform1fv(prog_light_pass->getUniform("light_omni_radius"), count, (float*)light_omni_radius);
            glUniform1i(prog_light_pass->getUniform("light_omni_count"), count);
        }
        gl::bindTexture2d(gl::TEXTURE_ALBEDO, g_buffer->getAlbedoTexture());
        gl::bindTexture2d(gl::TEXTURE_NORMAL, g_buffer->getNormalTexture());
        gl::bindTexture2d(gl::TEXTURE_METALLIC, g_buffer->getMetallicTexture());
        gl::bindTexture2d(gl::TEXTURE_ROUGHNESS, g_buffer->getRoughnessTexture());
        gl::bindCubeMap(gl::TEXTURE_ENVIRONMENT, irradiance_map->getId());
        gl::bindTexture2d(gl::TEXTURE_DEPTH, g_buffer->getDepthTexture());
        gl::bindTexture2d(gl::TEXTURE_EXT0, tex_ibl_brdf_lut->GetGlName());
        gl::bindCubeMap(gl::TEXTURE_EXT1, env_map->getId());

        gfxm::vec4 view_pos4 = gfxm::inverse(view) * gfxm::vec4(0,0,0,1);
        glUniform3f(prog_light_pass->getUniform("view_pos"), view_pos4.x, view_pos4.y, view_pos4.z);
        glUniform2f(prog_light_pass->getUniform("viewport_size"), (float)w, (float)h);
        gfxm::mat4 inverse_view_projection =
            gfxm::inverse(proj * view);
        glUniformMatrix4fv(prog_light_pass->getUniform("inverse_view_projection"), 1, GL_FALSE, (float*)&inverse_view_projection);
        drawQuad();

        endFrame();
    }
private:
    // Transient
    gfxm::mat4 proj;
    gfxm::mat4 view;
    GBuffer* g_buffer = 0;
    float w, h;
    // =========

    gl::ShaderProgram* prog_gbuf_solid;
    gl::ShaderProgram* prog_gbuf_skin;
    gl::ShaderProgram* prog_light_pass;
    gl::ShaderProgram* prog_skybox;

    std::shared_ptr<Texture2D> texture_white_px;
    std::shared_ptr<Texture2D> texture_black_px;
    std::shared_ptr<Texture2D> texture_normal_px;
    std::shared_ptr<Texture2D> texture_def_roughness_px;
    std::shared_ptr<Texture2D> tex_ibl_brdf_lut;
    std::shared_ptr<CubeMap> env_map;
    std::shared_ptr<CubeMap> irradiance_map;
};

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

#endif
