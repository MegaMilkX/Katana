#ifndef GL_CUBE_MAP_HPP
#define GL_CUBE_MAP_HPP


#include "glextutil.h"
#include "error.hpp"

namespace gl {

class CubeMap {
public:
    CubeMap();
    CubeMap(int width, int height, GLint internal_format, GLenum format);
    ~CubeMap();

    void init(int width, int height, GLint internal_format, GLenum format);

    GLuint getId() const { return glId; }

private:
    GLuint glId;
    GLint internal_format;
};

}


#endif
