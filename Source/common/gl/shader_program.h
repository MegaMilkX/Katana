#ifndef GL_SHADER_PROGRAM_H
#define GL_SHADER_PROGRAM_H

#include "shader.h"

namespace gl {

class ShaderProgram {
public:
    ShaderProgram();
    ~ShaderProgram();
    void attachShader(Shader* shader);
    void bindAttrib(GLuint loc, const std::string& name);
    void bindFragData(GLuint loc, const std::string& name);
    bool link();
    bool linkAndInit();
    GLuint getUniform(const std::string& name);
    GLuint locProjection() const { return loc_projection; }
    GLuint locView() const { return loc_view; }
    GLuint locModel() const { return loc_model; }
    void use();
    bool validate();
    GLuint getId() const;
private:
    ShaderProgram(const ShaderProgram &) {}
    ShaderProgram& operator=(const ShaderProgram &) {}

    GLuint id;
    GLuint loc_projection;
    GLuint loc_view;
    GLuint loc_model;
};

}

#endif
