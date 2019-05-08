#ifndef SHADER_FACTORY_HPP
#define SHADER_FACTORY_HPP

#include <string>
#include <memory>
#include <map>
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
        const std::string& fsource
    ) {
        return instance->_getOrCreate(
            name, vsource, fsource
        );
    }

    gl::ShaderProgram* _getOrCreate(
        const std::string& name, 
        const std::string& vsource, 
        const std::string& fsource
    ) {
        auto it = programs.find(name);
        if(it != programs.end()) {
            return it->second.get();
        } else {
            std::shared_ptr<gl::ShaderProgram>& prog = 
                programs[name];
            prog.reset(new gl::ShaderProgram());

            gl::Shader vs(GL_VERTEX_SHADER);
            gl::Shader fs(GL_FRAGMENT_SHADER);
            vs.source(vsource);
            fs.source(fsource);
            if(!vs.compile()) {
                LOG_WARN("Failed to compile vertex shader for program '" << name << "'");
                return 0;
            }
            if(!fs.compile()) {
                LOG_WARN("Failed to compile fragment shader for program '" << name << "'");
                return 0;
            }

            prog->attachShader(&vs);
            prog->attachShader(&fs);

            for(int i = gl::VERTEX_ATTRIB_FIRST; i < gl::VERTEX_ATTRIB_COUNT; ++i) {
                prog->bindAttrib(i, gl::getAttribDesc((gl::ATTRIB_INDEX)i).name);
            }
            prog->bindFragData(GBuffer::ALBEDO, "out_albedo");
            prog->bindFragData(GBuffer::NORMAL, "out_normal");
            prog->bindFragData(GBuffer::METALLIC, "out_metallic");
            prog->bindFragData(GBuffer::ROUGHNESS, "out_roughness");
            prog->bindFragData(0, "out_frag");
            if(!prog->link()) {
                LOG_WARN("Failed to link program '" << name << "'");
                return 0;
            }
            prog->use();

            GLuint u = 0;
            if((u = prog->getUniform("tex_diffuse"))) glUniform1i(u, gl::TEXTURE_DIFFUSE);
            if((u = prog->getUniform("tex_albedo"))) glUniform1i(u, gl::TEXTURE_ALBEDO);
            if((u = prog->getUniform("tex_normal"))) glUniform1i(u, gl::TEXTURE_NORMAL);
            if((u = prog->getUniform("tex_metallic"))) glUniform1i(u, gl::TEXTURE_METALLIC);
            if((u = prog->getUniform("tex_roughness"))) glUniform1i(u, gl::TEXTURE_ROUGHNESS);
            if((u = prog->getUniform("tex_position"))) glUniform1i(u, gl::TEXTURE_POSITION);
            if((u = prog->getUniform("tex_environment"))) glUniform1i(u, gl::TEXTURE_ENVIRONMENT);
            if((u = prog->getUniform("tex_depth"))) glUniform1i(u, gl::TEXTURE_DEPTH);

            if((u = prog->getUniform("tex_0"))) glUniform1i(u, gl::TEXTURE_0);
            if((u = prog->getUniform("tex_1"))) glUniform1i(u, gl::TEXTURE_1);
            if((u = prog->getUniform("tex_2"))) glUniform1i(u, gl::TEXTURE_2);
            if((u = prog->getUniform("tex_3"))) glUniform1i(u, gl::TEXTURE_3);
            if((u = prog->getUniform("tex_4"))) glUniform1i(u, gl::TEXTURE_4);
            if((u = prog->getUniform("tex_5"))) glUniform1i(u, gl::TEXTURE_5);
            if((u = prog->getUniform("tex_6"))) glUniform1i(u, gl::TEXTURE_6);

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
