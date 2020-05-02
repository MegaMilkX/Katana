#ifndef SHADER_FACTORY_HPP
#define SHADER_FACTORY_HPP

#include <string>
#include <memory>
#include <map>
#include <assert.h>
#include "gl/shader_program.h"
#include "gl/indexed_mesh.hpp"
#include "gl/texture.hpp"
#include "g_buffer.hpp"
#include "util/log.hpp"

class ShaderFactory {
public:
    static void init() {
        instance = new ShaderFactory();
    }
    static void cleanup() {
        delete instance;
    }
    static gl::ShaderProgram* getOrCreate(
        const std::string& name,
        const std::string& vsource,
        const std::string& fsource,
        bool force_reload = false
    ) {
        return instance->_getOrCreate(
            name, vsource, fsource, force_reload
        );
    }

    gl::ShaderProgram* _getOrCreate(
        const std::string& name, 
        const std::string& vsource, 
        const std::string& fsource,
        bool force_reload = false
    ) {
        auto it = programs.find(name);
        if(it != programs.end() && !force_reload) {
            return it->second.get();
        } else {
            std::shared_ptr<gl::ShaderProgram>& prog = 
                programs[name];
            if(!prog) {
                prog.reset(new gl::ShaderProgram());
            } else {
                prog->detachAll();
            }
            gl::Shader vs(GL_VERTEX_SHADER);
            gl::Shader fs(GL_FRAGMENT_SHADER);
            vs.source(vsource);
            fs.source(fsource);
            if(!vs.compile()) {
                LOG_WARN("Failed to compile vertex shader for program '" << name << "'");
                assert(false);
                return 0;
            }
            if(!fs.compile()) {
                LOG_WARN("Failed to compile fragment shader for program '" << name << "'");
                assert(false);
                return 0;
            }

            prog->attachShader(&vs);
            prog->attachShader(&fs);

            for(int i = 0; i < VERTEX_FMT::GENERIC::attribCount(); ++i) {
                prog->bindAttrib(i, VERTEX_FMT::GENERIC::getAttribDesc(i).name);
            }
            prog->bindFragData(GBuffer::ALBEDO, "out_albedo");
            prog->bindFragData(GBuffer::NORMAL, "out_normal");
            prog->bindFragData(GBuffer::METALLIC, "out_metallic");
            prog->bindFragData(GBuffer::ROUGHNESS, "out_roughness");
            prog->bindFragData(GBuffer::LIGHTNESS, "out_lightness");
            prog->bindFragData(0, "out_frag");
            if(!prog->link()) {
                LOG_WARN("Failed to link program '" << name << "'");
                assert(false);
                return 0;
            }
            prog->use();

            GLuint u = 0;
            if((u = prog->getUniform("tex_diffuse")) != -1) glUniform1i(u, gl::TEXTURE_DIFFUSE);
            if((u = prog->getUniform("tex_albedo")) != -1) glUniform1i(u, gl::TEXTURE_ALBEDO);
            if((u = prog->getUniform("tex_normal")) != -1) glUniform1i(u, gl::TEXTURE_NORMAL);
            if((u = prog->getUniform("tex_metallic")) != -1) glUniform1i(u, gl::TEXTURE_METALLIC);
            if((u = prog->getUniform("tex_roughness")) != -1) glUniform1i(u, gl::TEXTURE_ROUGHNESS);
            if((u = prog->getUniform("tex_lightness")) != -1) glUniform1i(u, gl::TEXTURE_LIGHTNESS);
            if((u = prog->getUniform("tex_position")) != -1) glUniform1i(u, gl::TEXTURE_POSITION);
            if((u = prog->getUniform("tex_environment")) != -1) glUniform1i(u, gl::TEXTURE_ENVIRONMENT);
            if((u = prog->getUniform("tex_depth")) != -1) glUniform1i(u, gl::TEXTURE_DEPTH);
            if((u = prog->getUniform("tex_ext0")) != -1) glUniform1i(u, gl::TEXTURE_EXT0);
            if((u = prog->getUniform("tex_ext1")) != -1) glUniform1i(u, gl::TEXTURE_EXT1);
            if((u = prog->getUniform("tex_ext2")) != -1) glUniform1i(u, gl::TEXTURE_EXT2);
            if((u = prog->getUniform("tex_shadowmap_omni[0]")) != -1) glUniform1i(u, gl::TEXTURE_SHADOW_CUBEMAP_0);
            if((u = prog->getUniform("tex_shadowmap_omni[1]")) != -1) glUniform1i(u, gl::TEXTURE_SHADOW_CUBEMAP_1);
            if((u = prog->getUniform("tex_shadowmap_omni[2]")) != -1) glUniform1i(u, gl::TEXTURE_SHADOW_CUBEMAP_2);
            if((u = prog->getUniform("tex_shadowmap_cube")) != -1) glUniform1i(u, gl::TEXTURE_SHADOWMAP_CUBE);
            if((u = prog->getUniform("tex_lightmap")) != -1) glUniform1i(u, gl::TEXTURE_LIGHTMAP);

            if((u = prog->getUniform("tex_0")) != -1) glUniform1i(u, gl::TEXTURE_0);
            if((u = prog->getUniform("tex_1")) != -1) glUniform1i(u, gl::TEXTURE_1);
            if((u = prog->getUniform("tex_2")) != -1) glUniform1i(u, gl::TEXTURE_2);
            if((u = prog->getUniform("tex_3")) != -1) glUniform1i(u, gl::TEXTURE_3);
            if((u = prog->getUniform("tex_4")) != -1) glUniform1i(u, gl::TEXTURE_4);
            if((u = prog->getUniform("tex_5")) != -1) glUniform1i(u, gl::TEXTURE_5);
            if((u = prog->getUniform("tex_6")) != -1) glUniform1i(u, gl::TEXTURE_6);
            if((u = prog->getUniform("tex_7")) != -1) glUniform1i(u, gl::TEXTURE_7);
            if((u = prog->getUniform("tex_8")) != -1) glUniform1i(u, gl::TEXTURE_8);
            if((u = prog->getUniform("tex_9")) != -1) glUniform1i(u, gl::TEXTURE_9);

            GLuint ublock = glGetUniformBlockIndex(prog->getId(), "uCommon3d_t");
            if(ublock != GL_INVALID_INDEX) {
                glUniformBlockBinding(prog->getId(), ublock, 0);
            }
            ublock = glGetUniformBlockIndex(prog->getId(), "uBones_t");
            if(ublock != GL_INVALID_INDEX) {
                glUniformBlockBinding(prog->getId(), ublock, 1);
            }

            if(!prog->validate()) {
                LOG_WARN("Failed to validate shader program '" << name << "'");
                assert(false);
            }
            glUseProgram(0);

            return prog.get();
        }
    }

    void clear() {
        programs.clear();
    }
private:
    static ShaderFactory* instance;

    std::map<std::string, std::shared_ptr<gl::ShaderProgram>> programs;
};

#endif
