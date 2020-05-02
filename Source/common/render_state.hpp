#ifndef RENDER_STATE_HPP
#define RENDER_STATE_HPP


#include "gfxm.hpp"
#include "shader_factory.hpp"
#include "resource/texture2d.h"
#include "resource/cube_map.hpp"
#include "resource/resource_tree.hpp"
#include "gl/uniform_buffers.hpp"

#define SKIN_BONE_LIMIT 256

struct uCommon3d {
    gfxm::mat4 view;
    gfxm::mat4 projection;
};
struct uBones {
    gfxm::mat4 pose[SKIN_BONE_LIMIT];
};

class RenderState {
    gl::ShaderProgram* prog = 0;

    // Defaults
    std::shared_ptr<Texture2D> texture_white_px;
    std::shared_ptr<Texture2D> texture_black_px;
    std::shared_ptr<Texture2D> texture_normal_px;
    std::shared_ptr<Texture2D> texture_def_roughness_px;
    std::shared_ptr<Texture2D> texture_def_metallic_px;
public:
    gl::UniformBuffer<uCommon3d, 0> ubufCommon3d;
    gl::UniformBuffer<uBones, 1> ubufBones;

    RenderState() {
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

        texture_def_metallic_px.reset(new Texture2D());
        unsigned char mtl[3] = { 0, 0, 0 };
        texture_def_metallic_px->Data(mtl, 1, 1, 3);
    }

    void bindUniformBuffers() {
        ubufCommon3d.bind();
        ubufBones.bind();
    }

    void setProgram(gl::ShaderProgram* p) {
        if(!p) {
            LOG_WARN("useProgram: program is null");
            return;
        }
        prog = p;
        prog->use();
    }
    void setMaterial(const Material* m) {
        if(!prog) return;

        GLuint textures[4] = {
            texture_white_px->GetGlName(),
            texture_normal_px->GetGlName(),
            texture_def_metallic_px->GetGlName(),
            texture_def_roughness_px->GetGlName()
        };
        gfxm::vec3 tint(1,1,1);
        if(m) {
            if(m->albedo) textures[0] = m->albedo->GetGlName();
            if(m->normal) textures[1] = m->normal->GetGlName();
            if(m->metallic) textures[2] = m->metallic->GetGlName();
            if(m->roughness) textures[3] = m->roughness->GetGlName();
            tint = m->tint;
        }
        gl::bindTexture2d(gl::TEXTURE_ALBEDO, textures[0]);
        gl::bindTexture2d(gl::TEXTURE_NORMAL, textures[1]);
        gl::bindTexture2d(gl::TEXTURE_METALLIC, textures[2]);
        gl::bindTexture2d(gl::TEXTURE_ROUGHNESS, textures[3]);

        //glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        //glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        
        glUniform3f(prog->getUniform("u_tint"), tint.x, tint.y, tint.z);
    }

    gl::ShaderProgram* getProgram() {
        return prog;
    }
};

#endif
