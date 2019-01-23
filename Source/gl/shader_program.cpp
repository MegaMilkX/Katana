#include "shader_program.h"

#include <iostream>
#include <vector>

#include "../util/log.hpp"

#include "indexed_mesh.hpp"

namespace gl {

ShaderProgram::ShaderProgram() {
    id = glCreateProgram();
    GL_LOG_ERROR("glCreateProgram");
}
ShaderProgram::~ShaderProgram() {
    glUseProgram(0);
    glDeleteProgram(id);
    GL_LOG_ERROR("glDeleteProgram");
}

void ShaderProgram::attachShader(Shader* shader)
{
    shader->attach(id);
}
void ShaderProgram::bindAttrib(GLuint loc, const std::string& name)
{
    glBindAttribLocation(id, loc, name.c_str());
    GL_LOG_ERROR("glBindAttribLocation");
}
void ShaderProgram::bindFragData(GLuint loc, const std::string& name)
{
    glBindFragDataLocation(id, loc, name.c_str());
    GL_LOG_ERROR("glBindFragDataLocation");
}
bool ShaderProgram::link()
{
    glLinkProgram(id);
    GL_LOG_ERROR("glLinkProgram");
    int res;
    int infoLogLen;
    glValidateProgram(id);
    GL_LOG_ERROR("glValidateProgram");

    glGetProgramiv(id, GL_VALIDATE_STATUS, &res);
    glGetProgramiv(id, GL_INFO_LOG_LENGTH, &infoLogLen);
    if(infoLogLen > 1)
    {
        std::vector<char> errMsg(infoLogLen + 1);
        glGetProgramInfoLog(id, infoLogLen, NULL, &errMsg[0]);
        LOG("GLSL link: " << &errMsg[0]);
    }
    if(res == GL_FALSE)
        return false;
    return true;
}
bool ShaderProgram::linkAndInit() {
    for(int i = gl::VERTEX_ATTRIB_FIRST; i < gl::VERTEX_ATTRIB_COUNT; ++i) {
        bindAttrib(i, gl::getAttribDesc((gl::ATTRIB_INDEX)i).name);
    }
    bindFragData(0, "out_albedo");
    bindFragData(1, "out_normal");
    bindFragData(2, "out_specular");
    bindFragData(3, "out_emission");
    link();
    use();
    glUniform1i(getUniform("tex_diffuse"), 0);
    glUniform1i(getUniform("tex_normal"), 1);
    glUniform1i(getUniform("tex_specular"), 2);
    glUniform1i(getUniform("tex_emission"), 3);
    glUniform1i(getUniform("tex0"), 0);
    glUniform1i(getUniform("tex1"), 1);
    loc_projection = getUniform("mat_projection");
    loc_view = getUniform("mat_view");
    loc_model = getUniform("mat_model");
}
GLuint ShaderProgram::getUniform(const std::string& name)
{
    return glGetUniformLocation(id, name.c_str());
}
void ShaderProgram::use()
{
    glUseProgram(id);
    GL_LOG_ERROR("glUseProgram");  
}
GLuint ShaderProgram::getId() const
{
    return id;
}

}