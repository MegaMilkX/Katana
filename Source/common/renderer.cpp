#include "renderer.hpp"

#include "world.hpp"

#include "attributes/render_environment.hpp"

int dbg_renderBufferId = 0;

void Renderer::drawSilhouettes(gl::FrameBuffer* fb, const DrawList& dl) {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    
    fb->bind();
    glClearColor(.0f, .0f, .0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    prog_silhouette_solid->use();
    glUniform4fv(prog_silhouette_solid->getUniform("u_color"), 1, (float*)&gfxm::vec4(1,1,1,1));
    drawMultiple(
        prog_silhouette_solid,
        dl.solids.data(),
        dl.solids.size()
    );
    prog_silhouette_skin->use();
    glUniform4fv(prog_silhouette_skin->getUniform("u_color"), 1, (float*)&gfxm::vec4(1,1,1,1));
    drawMultiple(
        prog_silhouette_skin,
        dl.skins.data(),
        dl.skins.size()
    );
}

void Renderer::drawPickPuffer(gl::FrameBuffer* fb, const DrawList& dl) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glBlendFunc(GL_ONE, GL_ZERO);
    glCullFace(GL_BACK);

    fb->bind();
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
    drawMultiplePick(
        prog_pick_skin,
        dl.skins.data(),
        dl.skins.size(),
        dl.solids.size()
    );
}

void Renderer::drawWorld(RenderViewport* vp, ktWorld* world) {
    DrawList dl;
    world->getRenderController()->getDrawList(dl);

    Camera* cam = world->getRenderController()->getDefaultCamera();
    static gfxm::mat4 proj = gfxm::perspective(1.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    static gfxm::mat4 view = gfxm::inverse(gfxm::mat4(1.0f));
    if(cam) {
        proj = cam->getProjection(vp->getWidth(), vp->getHeight());
        view = cam->getView();
    }

    auto env = world->getScene()->get<RenderEnvironment>();
    setSkyGradient(env->getSkyGradient());

    draw(vp, proj, view, dl);
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

void Renderer::drawToScreen(GLuint textureId) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);
    prog_quad->use();
    gl::bindTexture2d(gl::TEXTURE_ALBEDO, textureId);
    drawQuad();
}

void Renderer::beginFrame(RenderViewport* vp, gfxm::mat4& proj, gfxm::mat4& view) {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    vp->getGBuffer()->bind();
    glViewport(0, 0, (GLsizei)vp->getWidth(), (GLsizei)vp->getHeight());

    getState().ubufCommon3d.upload(uCommon3d{view, proj});

    getState().bindUniformBuffers();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::endFrame(RenderViewport* vp, bool draw_final_on_screen, bool draw_skybox) {
    // ==== Skybox ========================
    
    if(draw_skybox) {
        vp->getFinalBuffer()->bind();
        glDisable(GL_CULL_FACE);
        glDepthFunc(GL_LEQUAL);
        prog_skybox->use();
        gl::bindCubeMap(gl::TEXTURE_ENVIRONMENT, env_map->getId());
        drawCube();
    }

    // ==== 
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
    }

    glUseProgram(0);
}

// == PBR ===

RendererPBR::RendererPBR() {
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
}

void RendererPBR::draw(RenderViewport* vp, gfxm::mat4& proj, gfxm::mat4& view, const DrawList& draw_list, bool draw_final_on_screen, bool draw_skybox) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBlendFunc(GL_ONE, GL_ZERO);
    glDepthMask(GL_TRUE);
    glDisable(GL_SCISSOR_TEST);

    beginFrame(vp, proj, view);
/**/
    drawMultiple(
        prog_gbuf_solid,
        draw_list.solids.data(),
        draw_list.solids.size()
    );
    drawMultiple(
        prog_gbuf_skin,
        draw_list.skins.data(),
        draw_list.skins.size()
    );
    
    // ==== Light pass ===============
    vp->getFinalBuffer()->bind();
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    prog_light_pass->use();
    
    const int MAX_OMNI_LIGHT = 10;
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

    auto g_buffer = vp->getGBuffer();
    gl::bindTexture2d(gl::TEXTURE_ALBEDO, g_buffer->getAlbedoTexture());
    gl::bindTexture2d(gl::TEXTURE_NORMAL, g_buffer->getNormalTexture());
    gl::bindTexture2d(gl::TEXTURE_METALLIC, g_buffer->getMetallicTexture());
    gl::bindTexture2d(gl::TEXTURE_ROUGHNESS, g_buffer->getRoughnessTexture());
    gl::bindCubeMap(gl::TEXTURE_ENVIRONMENT, irradiance_map->getId());
    gl::bindTexture2d(gl::TEXTURE_DEPTH, g_buffer->getDepthTexture());
    gl::bindTexture2d(gl::TEXTURE_EXT0, tex_ibl_brdf_lut->GetGlName());
    gl::bindCubeMap(gl::TEXTURE_EXT1, env_map->getId());

    auto w = vp->getWidth();
    auto h = vp->getHeight();
    gfxm::vec4 view_pos4 = gfxm::inverse(view) * gfxm::vec4(0,0,0,1);
    glUniform3f(prog_light_pass->getUniform("view_pos"), view_pos4.x, view_pos4.y, view_pos4.z);
    glUniform2f(prog_light_pass->getUniform("viewport_size"), (float)w, (float)h);
    gfxm::mat4 inverse_view_projection =
        gfxm::inverse(proj * view);
    glUniformMatrix4fv(prog_light_pass->getUniform("inverse_view_projection"), 1, GL_FALSE, (float*)&inverse_view_projection);
    drawQuad();
    
    endFrame(vp, draw_final_on_screen, draw_skybox);
}