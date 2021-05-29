#ifndef RENDERER_BASE_HPP
#define RENDERER_BASE_HPP

#include "render_viewport.hpp"
#include "gfxm.hpp"
#include "gl/indexed_mesh.hpp"
#include "render/shader_loader.hpp"
#include "resource/texture2d.h"
#include "resource/cube_map.hpp"

#include "resource/resource_tree.hpp"

#include "draw_list.hpp"

#include "gl/uniform_buffers.hpp"
#include "gl/cube_map.hpp"

#include "util/animation/curve.h"

#include "data_headers/ibl_brdf_lut.png.h"

#include "render_state.hpp"

#include "draw_primitive.hpp"

extern int dbg_renderBufferId;

class ktWorld;
class Renderer {
protected:
    std::shared_ptr<Texture2D> texture_white_px;
    std::shared_ptr<Texture2D> texture_black_px;

public:
    Renderer() {
        {
            env_map.reset(new CubeMap());
            irradiance_map.reset(new CubeMap());

            curve<gfxm::vec3> curv;/*
            curv[0.0f] = gfxm::vec3(0 / 255.0f, 0 / 255.0f, 0 / 255.0f);
            curv[.5f] = gfxm::vec3(3 / 255.0f, 3 / 255.0f, 5 / 255.0f);
            curv[.55f] = gfxm::vec3(255 / 255.0f, 74 / 255.0f, 5 / 255.0f); 
            curv[.60f] = gfxm::vec3(160 / 255.0f, 137 / 255.0f, 129 / 255.0f);
            curv[.7f] = gfxm::vec3(1 / 255.0f, 44 / 255.0f, 95 / 255.0f);
            curv[1.0f] = gfxm::vec3(1 / 255.0f, 11 / 255.0f, 36 / 255.0f);*/
            curv[0.0f] = gfxm::vec3(0 / 255.0f, 0 / 255.0f, 0 / 255.0f) * 0.8f; 
            curv[0.5f] = gfxm::vec3(70 / 255.0f, 70 / 255.0f, 70 / 255.0f) * 0.8f;
            curv[0.55f] = gfxm::vec3(140 / 255.0f, 140 / 255.0f, 140 / 255.0f) * 0.8f;
            curv[1.0f] = gfxm::vec3(100 / 255.0f, 100 / 255.0f, 100 / 255.0f) * 0.8f;
            setSkyGradient(curv);
        }
        
        tex_ibl_brdf_lut.reset(new Texture2D());
        dstream dstrm;
        dstrm.setBuffer(std::vector<char>(ibl_brdf_lut_png, ibl_brdf_lut_png + sizeof(ibl_brdf_lut_png)));
        tex_ibl_brdf_lut->deserialize(dstrm, sizeof(ibl_brdf_lut_png));

        prog_quad = shaderLoader().loadShaderProgram("shaders/screen_quad.glsl");
        prog_skybox = shaderLoader().loadShaderProgram("shaders/skybox.glsl");

        prog_silhouette_solid = shaderLoader().loadShaderProgram("shaders/silhouette_solid.glsl");
        prog_silhouette_skin = shaderLoader().loadShaderProgram("shaders/silhouette_skin.glsl");

        prog_pick_solid = shaderLoader().loadShaderProgram("shaders/pick_solid.glsl");
        prog_pick_skin = shaderLoader().loadShaderProgram("shaders/pick_skin.glsl");

        texture_white_px.reset(new Texture2D());
        unsigned char wht[3] = { 255, 255, 255 };
        texture_white_px->Data(wht, 1, 1, 3);

        texture_black_px.reset(new Texture2D());
        unsigned char blk[3] = { 0, 0, 0 };
        texture_black_px->Data(blk, 1, 1, 3);
    }
    virtual ~Renderer() {}

    RenderState& getState() { return state; }

    void setGlStates();
    void setupUniformBuffers(const gfxm::mat4& proj, const gfxm::mat4& view);
    void clear();

    void beginFrame(RenderViewport* vp, gfxm::mat4& proj, gfxm::mat4& view);
    void endFrame(GLint final_fb, const gfxm::mat4& view, const gfxm::mat4& proj, bool draw_skybox);

    void drawOne(const DrawCmdSolid& cmd, Texture2D* default_lightmap = 0) {
        getState().setMaterial(cmd.material);
        if(cmd.lightmap) {
            gl::bindTexture2d(gl::TEXTURE_LIGHTMAP, cmd.lightmap->GetGlName());
        } else {
            if(default_lightmap) {
                gl::bindTexture2d(gl::TEXTURE_LIGHTMAP, default_lightmap->GetGlName());
            } else {
                gl::bindTexture2d(gl::TEXTURE_LIGHTMAP, texture_white_px->GetGlName());
            }
        }

        gl::bindCubeMap(gl::TEXTURE_ENVIRONMENT, irradiance_map->getId());
        gl::bindCubeMap(gl::TEXTURE_EXT1, env_map->getId());

        cmd.bind(getState());

        glBindVertexArray(cmd.vao);
        glDrawElements(GL_TRIANGLES, cmd.indexCount, GL_UNSIGNED_INT, (GLvoid*)cmd.indexOffset);
        
        GLenum err = glGetError();
        assert(err == GL_NO_ERROR);
    }

    void drawMultiple(gl::ShaderProgram* p, const DrawCmdSolid* elements, size_t count, Texture2D* default_lightmap = 0) {
        getState().setProgram(p);
        for(size_t i = 0; i < count; ++i) {
            auto& e = elements[i];
            drawOne(e, default_lightmap);
        }
    }
    void drawMultipleIndirect(gl::ShaderProgram* p, const DrawCmdSolid* elements, const int* indices, size_t count, Texture2D* default_lightmap = 0) {
        getState().setProgram(p);
        for(size_t i = 0; i < count; ++i) {
            auto idx = indices[i];
            drawOne(elements[idx], default_lightmap);
        }
    }
    template<typename T>
    void drawMultiplePick(gl::ShaderProgram* p, const T* elements, size_t count, uint32_t base_id) {
        getState().setProgram(p);
        for(size_t i = 0; i < count; ++i) {
            auto& e = elements[i];
            e.bind(getState());

            uint32_t id = base_id + i;
            uint32_t r = (id & 0x000000FF) >> 0;
            uint32_t g = (id & 0x0000FF00) >> 8;
            uint32_t b = (id & 0x00FF0000) >> 16;

            glUniform4f(p->getUniform("object_ptr"), r / 255.0f, g / 255.0f, b / 255.0f, .0f);

            glBindVertexArray(e.vao);
            glDrawElements(GL_TRIANGLES, e.indexCount, GL_UNSIGNED_INT, (GLvoid*)e.indexOffset);
            GLenum err = glGetError();
            assert(err == GL_NO_ERROR);
        }
    }

    void drawSilhouettes(gl::FrameBuffer* fb, const gfxm::mat4& proj, const gfxm::mat4& view, const DrawList& dl);
    void drawPickPuffer(gl::FrameBuffer* fb, const gfxm::mat4& proj, const gfxm::mat4& view, const DrawList& dl);
    void drawToScreen(GLuint textureId);

    virtual void draw(RenderViewport* vp, const gfxm::mat4& proj, const gfxm::mat4& view, DrawList& draw_list, bool draw_final_on_screen = false, bool draw_skybox = true) = 0;

    void setSkyGradient(curve<gfxm::vec3> grad);
    void setSkyCubemap(std::shared_ptr<CubeMap> cubemap);
protected:
    RenderState state;

    gl::ShaderProgram* prog_quad;
    gl::ShaderProgram* prog_skybox;

    gl::ShaderProgram* prog_silhouette_solid;
    gl::ShaderProgram* prog_silhouette_skin;

    gl::ShaderProgram* prog_pick_solid;
    gl::ShaderProgram* prog_pick_skin;

    std::shared_ptr<Texture2D> tex_ibl_brdf_lut;
    std::shared_ptr<CubeMap> env_map;
    std::shared_ptr<CubeMap> irradiance_map;
};


#define RENDERER_PBR_SHADOW_CUBEMAP_COUNT 4
#define RENDERER_PBR_SHADOW_CUBEMAP_SIZE 512
#define RENDERER_PBR_SHADOW_MAP_SIZE 4096

class RendererPBR : public Renderer {
    gl::ShaderProgram* prog_gbuf_solid;
    gl::ShaderProgram* prog_gbuf_skin;
    gl::ShaderProgram* prog_light_omni;
    gl::ShaderProgram* prog_light_dir;
    gl::ShaderProgram* prog_deferred_final;
    gl::ShaderProgram* prog_lightmap_sample;

    gl::ShaderProgram* prog_shadowmap_solid;
    gl::ShaderProgram* prog_shadowmap_skin;

    gl::ShaderProgram* prog_screen_quad;

    GLuint      shadowmap;
    gl::CubeMap shadowmap_cube;

public:
    RendererPBR();
    ~RendererPBR();

    virtual void draw(RenderViewport* vp, const gfxm::mat4& proj, const gfxm::mat4& view, DrawList& draw_list, bool draw_final_on_screen = false, bool draw_skybox = true);
    void draw(GBuffer* gbuffer, const gfxm::ivec4& vp, const gfxm::mat4& proj, const gfxm::mat4& view, DrawList& draw_list, GLint final_framebuffer_id, bool draw_skybox = true);
    void sampleLightmap(const gfxm::ivec4& vp, const gfxm::mat4& proj, const gfxm::mat4& view, const DrawList& dl);
    void drawShadowMap(GLuint map, const gfxm::vec3& dir, gfxm::mat4& out_view, gfxm::mat4& out_proj, const DrawList& draw_list);
    void drawShadowCubeMap(gl::CubeMap* cube_map, const gfxm::vec3& world_pos, const DrawList& draw_list);
};

std::unique_ptr<gl::ShaderProgram> createSkinComputeShader();

void runDeformers(DrawList& dl);

#endif
