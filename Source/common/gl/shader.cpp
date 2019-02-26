#include "shader.h"

#include <iostream>
#include <vector>

#include "../util/log.hpp"

namespace gl {

Shader::Shader(GLenum type) {
    id = glCreateShader(type);
}
Shader::~Shader() {
    glDeleteShader(id);
}

void Shader::source(const std::string& src)
{
    const char* csrc = src.c_str();
    glShaderSource(id, 1, &csrc, 0);
}
bool Shader::compile()
{
    glCompileShader(id);
    GL_LOG_ERROR("glCompileShader");
    GLint res = GL_FALSE;
    int infoLogLen;
    glGetShaderiv(id, GL_COMPILE_STATUS, &res);
    glGetShaderiv(id, GL_INFO_LOG_LENGTH, &infoLogLen);
    if(infoLogLen > 1)
    {
        std::vector<char> errMsg(infoLogLen + 1);
        glGetShaderInfoLog(id, infoLogLen, NULL, &errMsg[0]);
        LOG("GLSL compile: " << &errMsg[0]);
    }
    if(res == GL_FALSE)
        return false;
    return true;
}
void Shader::attach(GLuint program)
{
    glAttachShader(program, id);
}

}