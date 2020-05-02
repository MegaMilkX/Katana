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
    TEXTURE_7,
    TEXTURE_8,
    TEXTURE_9,
    TEXTURE_10,
    TEXTURE_11,
    TEXTURE_12,
    TEXTURE_13,
    TEXTURE_14,
    TEXTURE_15,
    TEXTURE_16,
    TEXTURE_17,

    TEXTURE_DIFFUSE = TEXTURE_0,
    TEXTURE_ALBEDO = TEXTURE_0,
    TEXTURE_NORMAL,
    TEXTURE_METALLIC,
    TEXTURE_ROUGHNESS,
    TEXTURE_LIGHTNESS,
    TEXTURE_POSITION,
    TEXTURE_ENVIRONMENT,
    TEXTURE_DEPTH,
    TEXTURE_EXT0,
    TEXTURE_EXT1,
    TEXTURE_EXT2,
    TEXTURE_SHADOWMAP_CUBE,
    TEXTURE_SHADOW_CUBEMAP_0,
    TEXTURE_SHADOW_CUBEMAP_1,
    TEXTURE_SHADOW_CUBEMAP_2,
    TEXTURE_LIGHTMAP
};

inline void bindTexture2d(TEXTURE_UNIT u, GLuint t) {
    assert(t < GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS);
    glActiveTexture(GL_TEXTURE0 + (int)u);
    glBindTexture(GL_TEXTURE_2D, t);
    assert(glGetError() == GL_NO_ERROR);
}

inline void bindCubeMap(TEXTURE_UNIT u, GLuint t) {
    assert(t < GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS);
    glActiveTexture(GL_TEXTURE0 + (int)u);
    glBindTexture(GL_TEXTURE_CUBE_MAP, t);
    assert(glGetError() == GL_NO_ERROR);
}

}

#endif
