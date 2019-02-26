#ifndef GL_SHADER_H
#define GL_SHADER_H

#include <string>
#include "glextutil.h"
#include "error.hpp"

namespace gl {

class Shader {
public:
    Shader(GLenum type);
    ~Shader();
    void source(const std::string& src);
    bool compile();
    void attach(GLuint program);
private:
    Shader(const Shader &) {}
    Shader& operator=(const Shader &) {}

    GLuint id;
};

}

#endif
