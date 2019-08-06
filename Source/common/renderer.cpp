#include "renderer.hpp"

#include "world.hpp"

#include "attributes/render_environment.hpp"

int dbg_renderBufferId = 0;

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
