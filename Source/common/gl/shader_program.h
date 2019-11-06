#ifndef GL_SHADER_PROGRAM_H
#define GL_SHADER_PROGRAM_H

#include "shader.h"

#include <rttr/type>

namespace gl {

struct UniformInfo {
    std::string name;
    GLint loc;
    GLenum type;
};

class ShaderProgram {
public:
    ShaderProgram();
    ~ShaderProgram();
    void attachShader(Shader* shader);
    void bindAttrib(GLuint loc, const std::string& name);
    void bindFragData(GLuint loc, const std::string& name);
    bool link();
    GLuint getUniform(const std::string& name);
    GLuint locProjection() const { return loc_projection; }
    GLuint locView() const { return loc_view; }
    GLuint locModel() const { return loc_model; }
    void use();
    bool validate();
    GLuint getId() const;

    size_t uniformCount() const {
        return uniforms.size();
    }
    const UniformInfo& getUniformInfo(size_t i) const {
        return uniforms[i];
    }

    void uploadModelTransform(const gfxm::mat4& v) {
        glUniformMatrix4fv(loc_model, 1, GL_FALSE, (float*)&v);
    }
private:
    ShaderProgram(const ShaderProgram &) {}
    ShaderProgram& operator=(const ShaderProgram &) {}

    GLuint id;
    GLuint loc_projection;
    GLuint loc_view;
    GLuint loc_model;
    std::vector<UniformInfo> uniforms;
};

}

#endif
