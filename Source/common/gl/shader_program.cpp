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

    loc_projection = getUniform("mat_projection");
    loc_view = getUniform("mat_view");
    loc_model = getUniform("mat_model");

    return true;
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
bool ShaderProgram::validate() {
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
        LOG("GLSL Validate: " << &errMsg[0]);
    }
    if(res == GL_FALSE)
        return false;

    GLint count = 0;
    const GLsizei nameBufSize = 32;
    GLchar name[nameBufSize];
    GLsizei length;
    GLint size;
    GLenum type;
    glGetProgramiv(id, GL_ACTIVE_ATTRIBUTES, &count);
    LOG("Attribute count: " << count);
    for(int i = 0; i < count; ++i) {
        glGetActiveAttrib(id, (GLuint)i, nameBufSize, &length, &size, &type, name);
        LOG("Attrib " << i << ", type: " << type << ", name: " << name);
    }

    static std::map<GLenum, rttr::type> gl_type_to_rttr = {
        { GL_FLOAT_VEC3, rttr::type::get<gfxm::vec3>() },
        { GL_FLOAT_MAT4, rttr::type::get<gfxm::mat4>() }
    };

    glGetProgramiv(id, GL_ACTIVE_UNIFORMS, &count);
    LOG("Uniform count: " << count);
    for(int i = 0; i < count; ++i) {
        glGetActiveUniform(id, (GLuint)i, nameBufSize, &length, &size, &type, name);
        GLint loc = glGetUniformLocation(id, name);
        auto it = gl_type_to_rttr.find(type);
        if(it == gl_type_to_rttr.end()) continue;
        uniforms.push_back(UniformInfo{ name, loc, it->second });
        LOG("Uniform " << i << ", type: " << type << ", name: " << name << ", retrieved id: " << loc);
    }


    return true;
}
GLuint ShaderProgram::getId() const
{
    return id;
}

}