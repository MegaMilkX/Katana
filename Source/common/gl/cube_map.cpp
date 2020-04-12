#include "cube_map.hpp"


namespace gl {

CubeMap::CubeMap() {
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &glId);
    glBindTexture(GL_TEXTURE_CUBE_MAP, glId);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

CubeMap::CubeMap(int width, int height, GLint internal_format, GLenum format)
: CubeMap() {
    init(width, height, internal_format, format);
}

CubeMap::~CubeMap() {
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glDeleteTextures(1, &glId);
}

void CubeMap::init(int width, int height, GLint internal_format, GLenum format) {
    glBindTexture(GL_TEXTURE_CUBE_MAP, glId);
    for(unsigned i = 0; i < 6; ++i) {
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0, internal_format, width, height, 0, format, GL_UNSIGNED_BYTE, 0
        );
    }
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

}