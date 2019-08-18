#ifndef RENDERER_BASE_HPP
#define RENDERER_BASE_HPP

#include "render_viewport.hpp"
#include "gfxm.hpp"
#include "gl/indexed_mesh.hpp"
#include "shader_factory.hpp"
#include "resource/texture2d.h"
#include "resource/cube_map.hpp"

#include "resource/resource_tree.hpp"

#include "draw_list.hpp"

#include "gl/uniform_buffers.hpp"

#include "util/animation/curve.h"

#include "data_headers/ibl_brdf_lut.png.h"

#include "render_state.hpp"

#include "draw_primitive.hpp"

extern int dbg_renderBufferId;

class ktWorld;
class Renderer {
public:
    Renderer() {
        {
            env_map.reset(new CubeMap());
            irradiance_map.reset(new CubeMap());

            curve<gfxm::vec3> curv;
            curv[0.0f] = gfxm::vec3(0 / 255.0f, 0 / 255.0f, 0 / 255.0f);
            curv[.5f] = gfxm::vec3(3 / 255.0f, 3 / 255.0f, 5 / 255.0f);
            curv[.55f] = gfxm::vec3(255 / 255.0f, 74 / 255.0f, 5 / 255.0f);
            curv[.60f] = gfxm::vec3(160 / 255.0f, 137 / 255.0f, 129 / 255.0f);
            curv[.7f] = gfxm::vec3(1 / 255.0f, 44 / 255.0f, 95 / 255.0f);
            curv[1.0f] = gfxm::vec3(1 / 255.0f, 11 / 255.0f, 36 / 255.0f);
            setSkyGradient(curv);
        }
        
        tex_ibl_brdf_lut.reset(new Texture2D());
        dstream dstrm;
        dstrm.setBuffer(std::vector<char>(ibl_brdf_lut_png, ibl_brdf_lut_png + sizeof(ibl_brdf_lut_png)));
        tex_ibl_brdf_lut->deserialize(dstrm, sizeof(ibl_brdf_lut_png));

        prog_quad = ShaderFactory::getOrCreate(
            "screen_quad",
            #include "../common/shaders/v_screen_quad.glsl"
            ,
            #include "../common/shaders/f_plain_texture.glsl"
        );
        prog_skybox = ShaderFactory::getOrCreate(
            "skybox",
            #include "../common/shaders/v_skybox.glsl"
            ,
            #include "../common/shaders/f_skybox.glsl"
        );
    }
    virtual ~Renderer() {}

    RenderState& getState() { return state; }

    void beginFrame(RenderViewport* vp, gfxm::mat4& proj, gfxm::mat4& view) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glCullFace(GL_BACK);
        glEnable(GL_CULL_FACE);
        glBindFramebuffer(GL_FRAMEBUFFER, vp->getGBuffer()->getGlFramebuffer());
        glViewport(0, 0, (GLsizei)vp->getWidth(), (GLsizei)vp->getHeight());

        getState().ubufCommon3d.upload(uCommon3d{view, proj});

        getState().bindUniformBuffers();

        glClear(GL_DEPTH_BUFFER_BIT);
    }

    void endFrame(RenderViewport* vp, bool draw_final_on_screen) {
        // ==== Skybox ========================
        
        glDisable(GL_CULL_FACE);
        glDepthFunc(GL_LEQUAL);
        prog_skybox->use();
        gl::bindCubeMap(gl::TEXTURE_ENVIRONMENT, env_map->getId());
        drawCube();
 
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

    template<typename T>
    void drawMultiple(gl::ShaderProgram* p, const T* elements, size_t count) {
        getState().setProgram(p);
        for(size_t i = 0; i < count; ++i) {
            auto& e = elements[i];

            getState().setMaterial(e.material);

            e.bind(getState());

            glBindVertexArray(e.vao);
            glDrawElements(GL_TRIANGLES, e.indexCount, GL_UNSIGNED_INT, (GLvoid*)e.indexOffset);
        }
    }

    void drawWorld(RenderViewport* vp, ktWorld* world);
    void drawToScreen(GLuint textureId);

    virtual void draw(RenderViewport* vp, gfxm::mat4& proj, gfxm::mat4& view, const DrawList& draw_list, bool draw_final_on_screen = false) = 0;

    void setSkyGradient(curve<gfxm::vec3> grad);
protected:
    RenderState state;

    gl::ShaderProgram* prog_quad;
    gl::ShaderProgram* prog_skybox;

    std::shared_ptr<Texture2D> tex_ibl_brdf_lut;
    std::shared_ptr<CubeMap> env_map;
    std::shared_ptr<CubeMap> irradiance_map;
};  

class RendererPBR : public Renderer {
    gl::ShaderProgram* prog_gbuf_solid;
    gl::ShaderProgram* prog_gbuf_skin;
    gl::ShaderProgram* prog_light_pass;

public:
    RendererPBR();

    virtual void draw(RenderViewport* vp, gfxm::mat4& proj, gfxm::mat4& view, const DrawList& draw_list, bool draw_final_on_screen = false) {
        glDisable(GL_SCISSOR_TEST);

        beginFrame(vp, proj, view);

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
        glBindFramebuffer(GL_FRAMEBUFFER, vp->getFinalBuffer()->getId());
        
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

        endFrame(vp, draw_final_on_screen);
    }
};

#endif
