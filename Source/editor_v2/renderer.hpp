#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "render_viewport.hpp"
#include "../common/gfxm.hpp"
#include "../common/gl/indexed_mesh.hpp"
#include "../common/shader_factory.hpp"
#include "../common/resource/texture2d.h"
#include "../common/resource/cube_map.hpp"

#include "../common/resource/resource_factory.h"

#include "gfx_draw_object.hpp"
#include "draw_list.hpp"

#define SKIN_BONE_LIMIT 110

void drawQuad();

class Renderer {
public:
    Renderer() {
        cube_map = retrieve<CubeMap>("env.hdr");

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

    void beginFrame(RenderViewport* vp, gfxm::mat4& proj, gfxm::mat4& view) {
        this->proj = proj;
        this->view = view;
        g_buffer = vp->getGBuffer();
        w = (float)vp->getWidth();
        h = (float)vp->getHeight();
        glEnable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_FRAMEBUFFER, vp->getGBuffer()->getGlFramebuffer());
        glViewport(0, 0, (GLsizei)vp->getWidth(), (GLsizei)vp->getHeight());
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    void endFrame() {
        // ==== Skybox ========================
        glDepthFunc(GL_LEQUAL);
        prog_skybox->use();
        glUniformMatrix4fv(prog_skybox->getUniform("mat_projection"), 1, GL_FALSE, (float*)&proj);
        glUniformMatrix4fv(prog_skybox->getUniform("mat_view"), 1, GL_FALSE, (float*)&view);
        gl::bindCubeMap(gl::TEXTURE_ENVIRONMENT, cube_map->getId());
        drawCube();

        glUseProgram(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void drawSolidObjects(const DrawList& draw_list) {
        auto prog = prog_gbuf_solid;

        prog->use();
        glUniformMatrix4fv(prog->getUniform("mat_projection"), 1, GL_FALSE, (float*)&proj);
        glUniformMatrix4fv(prog->getUniform("mat_view"), 1, GL_FALSE, (float*)&view);
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_2D, texture_white_px->GetGlName());
        glActiveTexture(GL_TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_2D, texture_normal_px->GetGlName());
        glActiveTexture(GL_TEXTURE0 + 2);
        glBindTexture(GL_TEXTURE_2D, texture_black_px->GetGlName());
        glActiveTexture(GL_TEXTURE0 + 3);
        glBindTexture(GL_TEXTURE_2D, texture_white_px->GetGlName());
        glUniform3f(prog->getUniform("u_tint"), 1.0f, 1.0f, 1.0f);

        for(size_t i = 0; i < draw_list.solidCount(); ++i) {
            auto o = draw_list.getSolid(i);
            glBindVertexArray(o->vao);
            glUniformMatrix4fv(prog->getUniform("mat_model"), 1, GL_FALSE, (float*)&o->transform);    
            glDrawElements(GL_TRIANGLES, o->indexCount, GL_UNSIGNED_INT, 0);
        }
    }

    void drawSkinObjects(const DrawList& dl) {
        auto p = prog_gbuf_skin;
        p->use();
        glUniformMatrix4fv(p->getUniform("mat_projection"), 1, GL_FALSE, (float*)&proj);
        glUniformMatrix4fv(p->getUniform("mat_view"), 1, GL_FALSE, (float*)&view);
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_2D, texture_white_px->GetGlName());
        glActiveTexture(GL_TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_2D, texture_normal_px->GetGlName());
        glActiveTexture(GL_TEXTURE0 + 2);
        glBindTexture(GL_TEXTURE_2D, texture_black_px->GetGlName());
        glActiveTexture(GL_TEXTURE0 + 3);
        glBindTexture(GL_TEXTURE_2D, texture_white_px->GetGlName());
        glUniform3f(p->getUniform("u_tint"), 1.0f, 1.0f, 1.0f);

        for(size_t i = 0; i < dl.skinCount(); ++i) {
            auto o = dl.getSkin(i);

            GLuint loc = p->getUniform("inverseBindPose[0]");
            glUniformMatrix4fv(
                loc,
                (std::min)((unsigned)SKIN_BONE_LIMIT, (unsigned)o->bind_transforms.size()),
                GL_FALSE,
                (GLfloat*)o->bind_transforms.data()
            );
            loc = p->getUniform("bones[0]");
            glUniformMatrix4fv(
                loc,
                (std::min)((unsigned)SKIN_BONE_LIMIT, (unsigned)o->bone_transforms.size()),
                GL_FALSE,
                (GLfloat*)o->bone_transforms.data()
            );
            glBindVertexArray(o->vao);
            glUniformMatrix4fv(p->getUniform("mat_model"), 1, GL_FALSE, (float*)&o->transform);    
            glDrawElements(GL_TRIANGLES, o->indexCount, GL_UNSIGNED_INT, 0);
        }
    }

    void draw(RenderViewport* vp, gfxm::mat4& proj, gfxm::mat4& view, const DrawList& draw_list) {
        beginFrame(vp, proj, view);
        
        drawSolidObjects(draw_list);
        drawSkinObjects(draw_list);

        // ==== Light pass ===============
        glBindFramebuffer(GL_FRAMEBUFFER, vp->getFinalBuffer()->getId());
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
        int light_omni_count = 0;
        glUniform3fv(prog_light_pass->getUniform("light_omni_pos"), light_omni_count, (float*)light_omni_pos);
        glUniform3fv(prog_light_pass->getUniform("light_omni_col"), light_omni_count, (float*)light_omni_col);
        glUniform1fv(prog_light_pass->getUniform("light_omni_radius"), light_omni_count, (float*)light_omni_radius);
        glUniform1i(prog_light_pass->getUniform("light_omni_count"), light_omni_count);
        
        gl::bindTexture2d(gl::TEXTURE_ALBEDO, g_buffer->getAlbedoTexture());
        gl::bindTexture2d(gl::TEXTURE_NORMAL, g_buffer->getNormalTexture());
        gl::bindTexture2d(gl::TEXTURE_METALLIC, g_buffer->getMetallicTexture());
        gl::bindTexture2d(gl::TEXTURE_ROUGHNESS, g_buffer->getRoughnessTexture());
        gl::bindCubeMap(gl::TEXTURE_ENVIRONMENT, cube_map->getId());
        gl::bindTexture2d(gl::TEXTURE_DEPTH, g_buffer->getDepthTexture());

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
    std::shared_ptr<CubeMap> cube_map;
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
