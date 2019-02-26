#ifndef GL_TEXTURE_UNIT_HPP
#define GL_TEXTURE_UNIT_HPP

#include "glextutil.h"

namespace gl {

enum TEXTURE_UNIT {
    TEXTURE_0,
    TEXTURE_1,
    TEXTURE_2,
    TEXTURE_3,
    TEXTURE_4,
    TEXTURE_5,
    TEXTURE_6,
    TEXTURE_DIFFUSE = TEXTURE_0,
    TEXTURE_ALBEDO = TEXTURE_0,
    TEXTURE_NORMAL = TEXTURE_1,
    TEXTURE_METALLIC = TEXTURE_2,
    TEXTURE_ROUGHNESS = TEXTURE_3,
    TEXTURE_POSITION = TEXTURE_4,
    TEXTURE_ENVIRONMENT = TEXTURE_5,
    TEXTURE_DEPTH = TEXTURE_6
};

inline void bindTexture2d(TEXTURE_UNIT u, GLuint t) {
    glActiveTexture(GL_TEXTURE0 + (int)u);
    glBindTexture(GL_TEXTURE_2D, t);
}

inline void bindCubeMap(TEXTURE_UNIT u, GLuint t) {
    glActiveTexture(GL_TEXTURE0 + (int)u);
    glBindTexture(GL_TEXTURE_CUBE_MAP, t);
}

}

#endif
