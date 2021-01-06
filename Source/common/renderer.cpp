#include "renderer.hpp"

#include "attributes/render_environment.hpp"


int dbg_renderBufferId = 0;

void Renderer::drawSilhouettes(gl::FrameBuffer* fb, const gfxm::mat4& proj, const gfxm::mat4& view, const DrawList& dl) {
    setGlStates();
    setupUniformBuffers(proj, view);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glDisable(GL_CULL_FACE);
    
    fb->bind();
    glViewport(0,0,fb->getWidth(),fb->getHeight());
    glClearColor(.0f, .0f, .0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    prog_silhouette_solid->use();
    glUniform4fv(prog_silhouette_solid->getUniform("u_color"), 1, (float*)&gfxm::vec4(1,1,1,1));
    drawMultipleIndirect(
        prog_silhouette_solid,
        dl.solids.data(),
        dl.draw_groups[DRAW_GROUP_EDITOR_SELECTED].data(),
        dl.draw_groups[DRAW_GROUP_EDITOR_SELECTED].size()
    );
}

void Renderer::drawPickPuffer(gl::FrameBuffer* fb, const gfxm::mat4& proj, const gfxm::mat4& view, const DrawList& dl) {
    setGlStates();
    setupUniformBuffers(proj, view);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glBlendFunc(GL_ONE, GL_ZERO);
    glCullFace(GL_BACK);

    fb->bind();
    glViewport(0,0,fb->getWidth(),fb->getHeight());
    uint32_t id = -1;
    uint32_t r = (id & 0x000000FF) >> 0;
    uint32_t g = (id & 0x0000FF00) >> 8;
    uint32_t b = (id & 0x00FF0000) >> 16;
    glClearColor(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawMultiplePick(
        prog_pick_solid,
        dl.solids.data(),
        dl.solids.size(),
        0
    );
}

void Renderer::setSkyGradient(curve<gfxm::vec3> curv) {
    const size_t byte_count = 512 * 3;
    char color[byte_count];
    for(int i = 0; i < byte_count; i += 3) {
        float y = i / (float)byte_count;
        gfxm::vec3 c = curv.at(y);
        color[i] = c.x * 255;
        color[i + 1] = c.y * 255;
        color[i + 2] = c.z * 255;
    }
    //env_map = retrieve<CubeMap>("env.hdr");
    
    env_map->data(color, 1, 512, 3);
    irradiance_map->makeIrradianceMap(env_map);
}

void Renderer::setSkyCubemap(std::shared_ptr<CubeMap> cubemap) {
    env_map = cubemap;
    irradiance_map->makeIrradianceMap(cubemap);
}

void Renderer::drawToScreen(GLuint textureId) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);
    prog_quad->use();
    gl::bindTexture2d(gl::TEXTURE_ALBEDO, textureId);
    drawQuad();
}

void Renderer::setGlStates() {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ZERO);
    glDepthMask(GL_TRUE);
    glDisable(GL_SCISSOR_TEST);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glCullFace(GL_BACK);
    //glEnable(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);
}

void Renderer::setupUniformBuffers(const gfxm::mat4& proj, const gfxm::mat4& view) {
    getState().ubufCommon3d.upload(uCommon3d{view, proj});
    getState().bindUniformBuffers();
}

void Renderer::clear() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::beginFrame(RenderViewport* vp, gfxm::mat4& proj, gfxm::mat4& view) {
    assert(false, "DONT CALL");
    //vp->getGBuffer()->bind();
    //glViewport(0, 0, (GLsizei)vp->getWidth(), (GLsizei)vp->getHeight());
}

void Renderer::endFrame(GLint final_fb, const gfxm::mat4& view, const gfxm::mat4& proj, bool draw_skybox) {
    // ==== Skybox ========================
    
    if(draw_skybox) {
        glBindFramebuffer(GL_FRAMEBUFFER, final_fb);
        glDisable(GL_CULL_FACE);
        glDepthFunc(GL_LEQUAL);
        prog_skybox->use();
        getState().ubufCommon3d.upload(uCommon3d{view, proj});
        getState().bindUniformBuffers();
        gl::bindCubeMap(gl::TEXTURE_ENVIRONMENT, env_map->getId());
        drawCube();
    }

    // ==== 
    /*
    if(draw_final_on_screen) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_DEPTH_BUFFER_BIT);
        prog_quad->use();
        switch(dbg_renderBufferId) {
        case 0:
            gl::bindTexture2d(gl::TEXTURE_ALBEDO, vp->getFinalImage());
            break;
        case 1:
            gl::bindTexture2d(gl::TEXTURE_ALBEDO, vp->getGBuffer()->getAlbedoTexture());
            break;
        case 2:
            gl::bindTexture2d(gl::TEXTURE_ALBEDO, vp->getGBuffer()->getNormalTexture());
            break;
        case 3:
            gl::bindTexture2d(gl::TEXTURE_ALBEDO, vp->getGBuffer()->getRoughnessTexture());
            break;
        case 4:
            gl::bindTexture2d(gl::TEXTURE_ALBEDO, vp->getGBuffer()->getMetallicTexture());
            break;
        case 5:
            gl::bindTexture2d(gl::TEXTURE_ALBEDO, vp->getGBuffer()->getDepthTexture());
            break;
        }
        drawQuad();
    }*/

    glUseProgram(0);
}

// == PBR ===

RendererPBR::RendererPBR() {
    shadowmap_cube.init(
        RENDERER_PBR_SHADOW_CUBEMAP_SIZE, 
        RENDERER_PBR_SHADOW_CUBEMAP_SIZE,
        GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT
    );

    prog_light_omni = shaderLoader().loadShaderProgram("shaders/light_pass_omni.glsl");
    prog_light_dir  = shaderLoader().loadShaderProgram("shaders/light_pass_direct.glsl");
    prog_deferred_final = shaderLoader().loadShaderProgram("shaders/deferred_final.glsl");
    prog_gbuf_solid = shaderLoader().loadShaderProgram("shaders/gbuf_solid_pbr.glsl");
    prog_gbuf_skin = shaderLoader().loadShaderProgram("shaders/gbuf_skin_pbr.glsl");
    
    prog_shadowmap_solid = shaderLoader().loadShaderProgram("shaders/shadowmap_solid.glsl");
    prog_shadowmap_skin = shaderLoader().loadShaderProgram("shaders/shadowmap_skin.glsl");

    prog_lightmap_sample = shaderLoader().loadShaderProgram("shaders/lightmap_sample.glsl");
}

const int MAX_OMNI_LIGHT = 10;

void RendererPBR::draw(RenderViewport* vp, const gfxm::mat4& proj, const gfxm::mat4& view, DrawList& draw_list, bool draw_final_on_screen, bool draw_skybox) {
    draw(
        vp->getGBuffer(),
        gfxm::ivec4(0, 0, vp->getWidth(), vp->getHeight()),
        proj, view,
        draw_list, draw_final_on_screen ? 0 : vp->getFinalBuffer()->getId(), draw_skybox
    );
}

void RendererPBR::draw(GBuffer* gbuffer, const gfxm::ivec4& vp, const gfxm::mat4& proj, const gfxm::mat4& view, DrawList& draw_list, GLint final_framebuffer_id, bool draw_skybox) {
    runDeformers(draw_list);
    
    setGlStates();
    setupUniformBuffers(proj, view);
    
    gbuffer->bind(
        GBuffer::ALBEDO_BIT
        | GBuffer::NORMAL_BIT
        | GBuffer::METALLIC_BIT
        | GBuffer::ROUGHNESS_BIT
        | GBuffer::LIGHTNESS_BIT,
        true
    );
    glViewport((GLsizei)vp.x, (GLsizei)vp.y, (GLsizei)vp.z, (GLsizei)vp.w);

    glClearColor(0, 0, 0, 1.0f);
    clear();
/**/
    drawMultiple(
        prog_gbuf_solid,
        draw_list.solids.data(),
        draw_list.solids.size()
    );

    // ==== Lightness ================
    for(size_t i = 0; i < draw_list.dir_lights.size(); ++i) {
        auto& l = draw_list.dir_lights[i];
        // TODO: No shadows yet
        gbuffer->bind(GBuffer::LIGHTNESS_BIT, false);
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glDepthMask(GL_FALSE);
        glDisable(GL_DEPTH_TEST);
        glViewport((GLsizei)vp.x, (GLsizei)vp.y, (GLsizei)vp.z, (GLsizei)vp.w);

        prog_light_dir->use();
        gl::bindTexture2d(gl::TEXTURE_ALBEDO, gbuffer->getAlbedoTexture());
        gl::bindTexture2d(gl::TEXTURE_DEPTH, gbuffer->getDepthTexture());
        gl::bindTexture2d(gl::TEXTURE_NORMAL, gbuffer->getNormalTexture());
        gl::bindTexture2d(gl::TEXTURE_METALLIC, gbuffer->getMetallicTexture());
        gl::bindTexture2d(gl::TEXTURE_ROUGHNESS, gbuffer->getRoughnessTexture());
        {
            gfxm::vec4 view_pos4 = gfxm::inverse(view) * gfxm::vec4(0,0,0,1);
            glUniform3f(prog_light_dir->getUniform("view_pos"), view_pos4.x, view_pos4.y, view_pos4.z);
            glUniform2f(prog_light_dir->getUniform("viewport_size"), (float)vp.z, (float)vp.w);
            gfxm::mat4 inverse_view_projection =
                gfxm::inverse(proj * view);
            glUniformMatrix4fv(prog_light_dir->getUniform("inverse_view_projection"), 1, GL_FALSE, (float*)&inverse_view_projection);
        }
        auto& dir = l.dir;
        auto& intensity = l.intensity;
        auto& color = l.color * intensity;

        glUniform3fv(prog_light_dir->getUniform("light_dir"), 1, (GLfloat*)&dir);
        glUniform3fv(prog_light_dir->getUniform("light_col"), 1, (float*)&color);

        drawQuad();
    }
    for(size_t i = 0; i < draw_list.omnis.size(); ++i) {
        auto& l = draw_list.omnis[i];
        drawShadowCubeMap(&shadowmap_cube, l.translation, draw_list);

        gbuffer->bind(
            GBuffer::LIGHTNESS_BIT,
            false
        );

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        

        glDepthMask(GL_FALSE);
        glDisable(GL_DEPTH_TEST);
        glViewport((GLsizei)vp.x, (GLsizei)vp.y, (GLsizei)vp.z, (GLsizei)vp.w);
        prog_light_omni->use();

        gl::bindTexture2d(gl::TEXTURE_ALBEDO, gbuffer->getAlbedoTexture());
        gl::bindTexture2d(gl::TEXTURE_DEPTH, gbuffer->getDepthTexture());
        gl::bindTexture2d(gl::TEXTURE_NORMAL, gbuffer->getNormalTexture());
        gl::bindTexture2d(gl::TEXTURE_METALLIC, gbuffer->getMetallicTexture());
        gl::bindTexture2d(gl::TEXTURE_ROUGHNESS, gbuffer->getRoughnessTexture());
        gl::bindCubeMap(gl::TEXTURE_SHADOWMAP_CUBE, shadowmap_cube.getId());

        {
            gfxm::vec4 view_pos4 = gfxm::inverse(view) * gfxm::vec4(0,0,0,1);
            glUniform3f(prog_light_omni->getUniform("view_pos"), view_pos4.x, view_pos4.y, view_pos4.z);
            glUniform2f(prog_light_omni->getUniform("viewport_size"), (float)vp.z, (float)vp.w);
            gfxm::mat4 inverse_view_projection =
                gfxm::inverse(proj * view);
            glUniformMatrix4fv(prog_light_omni->getUniform("inverse_view_projection"), 1, GL_FALSE, (float*)&inverse_view_projection);
        }
        
        auto& pos = l.translation;
        auto& intensity = l.intensity;
        float radius = l.radius;
        gfxm::vec3 col = l.color * intensity;

        glUniform3fv(prog_light_omni->getUniform("light_omni_pos"), 1, (GLfloat*)&pos);
        glUniform3fv(prog_light_omni->getUniform("light_omni_col"), 1, (float*)&col);
        glUniform1fv(prog_light_omni->getUniform("light_omni_radius"), 1, (float*)&radius);

        drawQuad();
    }
    
    // ==== Final ====================

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ZERO);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, final_framebuffer_id);
    glViewport((GLsizei)vp.x, (GLsizei)vp.y, (GLsizei)vp.z, (GLsizei)vp.w);
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    prog_deferred_final->use();

    gl::bindTexture2d(gl::TEXTURE_ALBEDO, gbuffer->getAlbedoTexture());
    gl::bindTexture2d(gl::TEXTURE_LIGHTNESS, gbuffer->getLightnessTexture());
    gl::bindTexture2d(gl::TEXTURE_DEPTH, gbuffer->getDepthTexture());

    drawQuad();

    //gl::foo(view, proj);

    // ==== Light pass ===============
    /*
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, final_framebuffer_id);
    glViewport((GLsizei)vp.x, (GLsizei)vp.y, (GLsizei)vp.z, (GLsizei)vp.w);
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    prog_light_pass->use();
    
    gfxm::vec3 light_omni_pos[MAX_OMNI_LIGHT];
    gfxm::vec3 light_omni_col[MAX_OMNI_LIGHT];
    float light_omni_radius[MAX_OMNI_LIGHT];
    {
        int count = std::min((int)draw_list.omnis.size(), MAX_OMNI_LIGHT);
        for(size_t i = 0; i < count && i < MAX_OMNI_LIGHT; ++i) {
            auto& l = draw_list.omnis[i];
            light_omni_pos[i] = l.translation;
            light_omni_col[i] = l.color * l.intensity;
            light_omni_radius[i] = l.radius;
        }
        glUniform3fv(prog_light_pass->getUniform("light_omni_pos"), count, (float*)light_omni_pos);
        glUniform3fv(prog_light_pass->getUniform("light_omni_col"), count, (float*)light_omni_col);
        glUniform1fv(prog_light_pass->getUniform("light_omni_radius"), count, (float*)light_omni_radius);
        glUniform1i(prog_light_pass->getUniform("light_omni_count"), count);
    }
    const int MAX_DIR_LIGHT = 3;
    gfxm::vec3 light_dir[MAX_DIR_LIGHT];
    gfxm::vec3 light_dir_col[MAX_DIR_LIGHT];
    {
        int count = std::min((int)draw_list.dir_lights.size(), MAX_DIR_LIGHT);
        for(size_t i = 0; i < count; ++i) {
            auto& l = draw_list.dir_lights[i];
            light_dir[i] = l.dir;
            light_dir_col[i] = l.color * l.intensity;
        }
        glUniform3fv(prog_light_pass->getUniform("light_dir"), count, (float*)light_dir);
        glUniform3fv(prog_light_pass->getUniform("light_dir_col"), count, (float*)light_dir_col);
        glUniform1i(prog_light_pass->getUniform("light_dir_count"), count);
    }

    gl::bindTexture2d(gl::TEXTURE_ALBEDO, gbuffer->getAlbedoTexture());
    gl::bindTexture2d(gl::TEXTURE_NORMAL, gbuffer->getNormalTexture());
    gl::bindTexture2d(gl::TEXTURE_METALLIC, gbuffer->getMetallicTexture());
    gl::bindTexture2d(gl::TEXTURE_ROUGHNESS, gbuffer->getRoughnessTexture());
    gl::bindTexture2d(gl::TEXTURE_LIGHTNESS, gbuffer->getLightnessTexture());
    gl::bindCubeMap(gl::TEXTURE_ENVIRONMENT, irradiance_map->getId());
    gl::bindTexture2d(gl::TEXTURE_DEPTH, gbuffer->getDepthTexture());
    gl::bindTexture2d(gl::TEXTURE_EXT0, tex_ibl_brdf_lut->GetGlName());
    gl::bindCubeMap(gl::TEXTURE_EXT1, env_map->getId());

    gl::bindCubeMap(gl::TEXTURE_SHADOW_CUBEMAP_0, shadow_cubemaps[0].getId());
    gl::bindCubeMap(gl::TEXTURE_SHADOW_CUBEMAP_1, shadow_cubemaps[1].getId());
    gl::bindCubeMap(gl::TEXTURE_SHADOW_CUBEMAP_2, shadow_cubemaps[2].getId());

    auto w = vp.z;
    auto h = vp.w;
    gfxm::vec4 view_pos4 = gfxm::inverse(view) * gfxm::vec4(0,0,0,1);
    glUniform3f(prog_light_pass->getUniform("view_pos"), view_pos4.x, view_pos4.y, view_pos4.z);
    glUniform2f(prog_light_pass->getUniform("viewport_size"), (float)w, (float)h);
    gfxm::mat4 inverse_view_projection =
        gfxm::inverse(proj * view);
    glUniformMatrix4fv(prog_light_pass->getUniform("inverse_view_projection"), 1, GL_FALSE, (float*)&inverse_view_projection);
    drawQuad();*/
    
    
    endFrame(final_framebuffer_id, view, proj, draw_skybox);
}

void RendererPBR::sampleLightmap(const gfxm::ivec4& vp, const gfxm::mat4& proj, const gfxm::mat4& view, const DrawList& dl) {
    //glBindFramebuffer(GL_FRAMEBUFFER, target_fb);
    setGlStates();

    prog_lightmap_sample->use();
    setupUniformBuffers(proj, view);
    glViewport((GLsizei)vp.x, (GLsizei)vp.y, (GLsizei)vp.z, (GLsizei)vp.w);
    drawMultiple(
        prog_lightmap_sample,
        dl.solids.data(),
        dl.solids.size(),
        texture_black_px.get()
    );
}

void RendererPBR::drawShadowCubeMap(gl::CubeMap* cube_map, const gfxm::vec3& eye, const DrawList& draw_list)
{
    gfxm::mat4 proj = gfxm::perspective(gfxm::radian(90.0f), 1.0f, 0.1f, 1000.0f);
    gfxm::mat4 views[] = 
    {
        gfxm::lookAt(eye, eye+gfxm::vec3( 1.0f,  0.0f,  0.0f), gfxm::vec3(0.0f, -1.0f,  0.0f)),
        gfxm::lookAt(eye, eye+gfxm::vec3(-1.0f,  0.0f,  0.0f), gfxm::vec3(0.0f, -1.0f,  0.0f)),
        gfxm::lookAt(eye, eye+gfxm::vec3( 0.0f,  1.0f,  0.0f), gfxm::vec3(0.0f,  0.0f,  1.0f)),
        gfxm::lookAt(eye, eye+gfxm::vec3( 0.0f, -1.0f,  0.0f), gfxm::vec3(0.0f,  0.0f, -1.0f)),
        gfxm::lookAt(eye, eye+gfxm::vec3( 0.0f,  0.0f,  1.0f), gfxm::vec3(0.0f, -1.0f,  0.0f)),
        gfxm::lookAt(eye, eye+gfxm::vec3( 0.0f,  0.0f, -1.0f), gfxm::vec3(0.0f, -1.0f,  0.0f))
    };

    const int side = RENDERER_PBR_SHADOW_CUBEMAP_SIZE;

    GLuint capFbo;
    glGenFramebuffers(1, &capFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, capFbo);
     // Disable writes to the color buffer
    glDrawBuffer(GL_NONE);
    // Disable reads from the color buffer
    glReadBuffer(GL_NONE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X, cube_map->getId(), 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_WARN("Cube map fbo not complete!");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);    
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    glDisable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ZERO);
    glDepthMask(GL_TRUE);
    glViewport(0,0,side,side);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, capFbo);
    for(unsigned i = 0; i < 6; ++i) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cube_map->getId(), 0);
        GLenum err = glGetError();
        assert(err == GL_NO_ERROR);
        
        glClear(GL_DEPTH_BUFFER_BIT);
        
        getState().ubufCommon3d.upload(uCommon3d{(views[i]), proj});
        getState().bindUniformBuffers();

        drawMultiple(
            prog_shadowmap_solid,
            draw_list.solids.data(),
            draw_list.solids.size()
        );
    }

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    glDeleteFramebuffers(1, &capFbo);
}


std::unique_ptr<gl::ShaderProgram> createSkinComputeShader() {
    std::unique_ptr<gl::ShaderProgram> p(new gl::ShaderProgram);

    gl::Shader sh(GL_COMPUTE_SHADER);
    sh.source(R"(
        #version 450
        layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

        struct VEC3 {
            float x, y, z;
        };
        layout(std430, binding = 0) buffer Position {
            VEC3 Positions[];
        };
        layout(std430, binding = 1) buffer Normal {
            VEC3 Normals[];
        };
        layout(std430, binding = 2) buffer Tangent {
            VEC3 Tangents[];
        };
        layout(std430, binding = 3) buffer Bitangent {
            VEC3 Bitangents[];
        };
        layout(std430, binding = 4) buffer BoneIndex4Buf {
            vec4 BoneIndex4[];
        };
        layout(std430, binding = 5) buffer BoneWeight4Buf {
            vec4 BoneWeight4[];
        };
        layout(std430, binding = 6) buffer Pose {
            mat4 Poses[];
        };

        layout(std430, binding = 7) buffer out_Position {
            VEC3 out_Positions[];
        };
        layout(std430, binding = 8) buffer out_Normal {
            VEC3 out_Normals[];
        };
        layout(std430, binding = 9) buffer out_Tangent {
            VEC3 out_Tangents[];
        };
        layout(std430, binding = 10) buffer out_Bitangent {
            VEC3 out_Bitangents[];
        };

        
        void main() {
            uint gid = gl_GlobalInvocationID.x;

            vec3 P = vec3(Positions[gid].x, Positions[gid].y, Positions[gid].z);
            vec3 N = vec3(Normals[gid].x, Normals[gid].y, Normals[gid].z);
            vec3 T = vec3(Tangents[gid].x, Tangents[gid].y, Tangents[gid].z);
            vec3 B = vec3(Bitangents[gid].x, Bitangents[gid].y, Bitangents[gid].z);

            ivec4 bi = ivec4(
                int(BoneIndex4[gid].x), int(BoneIndex4[gid].y),
                int(BoneIndex4[gid].z), int(BoneIndex4[gid].w)
            );
            vec4 w = BoneWeight4[gid];

            mat4 m0 = Poses[bi.x];
            mat4 m1 = Poses[bi.y];
            mat4 m2 = Poses[bi.z];
            mat4 m3 = Poses[bi.w];

            vec3 NS = (
                m0 * vec4(N, 0.0) * w.x +
                m1 * vec4(N, 0.0) * w.y +
                m2 * vec4(N, 0.0) * w.z +
                m3 * vec4(N, 0.0) * w.w
            ).xyz;
            vec3 TS = (
                m0 * vec4(T, 0.0) * w.x +
                m1 * vec4(T, 0.0) * w.y +
                m2 * vec4(T, 0.0) * w.z +
                m3 * vec4(T, 0.0) * w.w
            ).xyz;
            vec3 BS = (
                m0 * vec4(B, 0.0) * w.x +
                m1 * vec4(B, 0.0) * w.y +
                m2 * vec4(B, 0.0) * w.z +
                m3 * vec4(B, 0.0) * w.w
            ).xyz;

            //NS = normalize(NS);
            //TS = normalize(TS);
            //BS = normalize(BS);

            vec4 PS = (
                m0 * vec4(P, 1.0) * w.x +
                m1 * vec4(P, 1.0) * w.y +
                m2 * vec4(P, 1.0) * w.z +
                m3 * vec4(P, 1.0) * w.w
            );

            out_Positions[gid].x = PS.x;
            out_Positions[gid].y = PS.y;
            out_Positions[gid].z = PS.z;
            out_Normals[gid].x = NS.x;
            out_Normals[gid].y = NS.y;
            out_Normals[gid].z = NS.z;
            out_Tangents[gid].x = TS.x;
            out_Tangents[gid].y = TS.y;
            out_Tangents[gid].z = TS.z;
            out_Bitangents[gid].x = BS.x;
            out_Bitangents[gid].y = BS.y;
            out_Bitangents[gid].z = BS.z;
        }
    )");
    if (!sh.compile()) {
        assert(false);
        return 0;
    }
    p->attachShader(&sh);
    if (!p->link()) {
        assert(false);
        return 0;
    }



    return p;
}

#include "resource/model/model.hpp"
void computeSkinDeform(
    MeshDeformerSkin* dfm,
    gfxm::mat4 root_m4
) {
    static std::unique_ptr<gl::ShaderProgram> p = createSkinComputeShader();

    glBindVertexArray(0);

    std::vector<gfxm::mat4> bone_transforms(dfm->bone_nodes.size());
    for(int i = 0; i < dfm->bone_nodes.size(); ++i) {
        auto node_id = dfm->bone_nodes[i];
        if(node_id >= 0) {
            bone_transforms[i] = dfm->model->getWorldTransform(node_id) * dfm->bind_transforms[i];
        } else {
            bone_transforms[i] = gfxm::mat4(1.0f);
        }
    }
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
    dfm->pose->upload(bone_transforms.data(), bone_transforms.size() * sizeof(gfxm::mat4));

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

    GLuint pos_id = dfm->getSourceMesh()->getAttribBuffer(VFMT::ENUM_GENERIC::Position)->getId();
    GLuint nrm_id = dfm->getSourceMesh()->getAttribBuffer(VFMT::ENUM_GENERIC::Normal)->getId();
    GLuint tan_id = dfm->getSourceMesh()->getAttribBuffer(VFMT::ENUM_GENERIC::Tangent)->getId();
    GLuint bitan_id = dfm->getSourceMesh()->getAttribBuffer(VFMT::ENUM_GENERIC::Bitangent)->getId();
    GLuint bi4_id = dfm->getSourceMesh()->getAttribBuffer(VFMT::ENUM_GENERIC::BoneIndex4)->getId();
    GLuint bw4_id = dfm->getSourceMesh()->getAttribBuffer(VFMT::ENUM_GENERIC::BoneWeight4)->getId();
    

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, pos_id);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, nrm_id);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, tan_id);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, bitan_id);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, bi4_id);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, bw4_id);
    dfm->pose->bindBase(GL_SHADER_STORAGE_BUFFER, 6);
    dfm->position->bindBase(GL_SHADER_STORAGE_BUFFER, 7);
    dfm->normal->bindBase(GL_SHADER_STORAGE_BUFFER, 8);
    dfm->tangent->bindBase(GL_SHADER_STORAGE_BUFFER, 9);
    dfm->bitangent->bindBase(GL_SHADER_STORAGE_BUFFER, 10);

    p->use();

    glDispatchCompute(
        dfm->getSourceMesh()->getAttribDataSize(VFMT::ENUM_GENERIC::Position) / VFMT::Position::elem_size / VFMT::Position::count, 
        1, 1
    );
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, 0);

    // Done!
}


void runDeformers(DrawList& dl) {
    auto& skin_deformers = dl.deformers[MESH_DEFORMER_SKIN];
    for (auto d : skin_deformers) {
        MeshDeformerSkin* dfm = (MeshDeformerSkin*)d.deformer;

        computeSkinDeform(dfm, gfxm::mat4(1.0f));
        dl.solids[d.drawCmd].vao = dfm->vao->getId();
        dl.solids[d.drawCmd].transform = gfxm::mat4(1.0f);
    }
}