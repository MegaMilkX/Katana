#ifndef SHADER_FACTORY_HPP
#define SHADER_FACTORY_HPP

#include <string>
#include <memory>
#include <map>
#include "gl/shader_program.h"
#include "gl/indexed_mesh.hpp"
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
            std::shared_ptr<gl::ShaderProgram> prog = 
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
            prog->bindFragData(GBuffer::POSITION, "out_position");
            prog->bindFragData(GBuffer::NORMAL, "out_normal");
            prog->bindFragData(GBuffer::METALLIC, "out_metallic");
            prog->bindFragData(GBuffer::ROUGHNESS, "out_roughness");
            prog->bindFragData(0, "out_frag");
            if(!prog->link()) {
                LOG_WARN("Failed to link program '" << name << "'");
            }
            prog->use();

            glUniform1i(prog->getUniform("tex_albedo"), 0);
            glUniform1i(prog->getUniform("tex_normal"), 1);
            glUniform1i(prog->getUniform("tex_metallic"), 2);
            glUniform1i(prog->getUniform("tex_roughness"), 3);
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
