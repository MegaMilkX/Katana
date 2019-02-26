#ifndef DRAW_HPP
#define DRAW_HPP

#include "gl/frame_buffer.hpp"
#include "gl/shader_program.h"

namespace gl {

struct DrawInfo {
    GLuint vao;
    GLsizei index_count;
    int offset;
    float transform[16];
    GLuint textures[4];
    uint64_t user_ptr;
};

inline void enable_flags_3d() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

inline void draw(
    GLuint framebuffer, 
    GLuint program, 
    GLsizei vp_width, 
    GLsizei vp_height, 
    GLuint loc_projection, 
    float* projection, 
    GLuint loc_view, 
    float* view,
    GLuint loc_model,
    DrawInfo* draw_list,
    size_t draw_count
) {
    glEnable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, vp_width, vp_height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // TODO: Should only clear depth

    glUseProgram(program);
    glUniformMatrix4fv(loc_projection, 1, GL_FALSE, projection);
    glUniformMatrix4fv(loc_view, 1, GL_FALSE, view);

    for(size_t i = 0; i < draw_count; ++i) {
        DrawInfo& d = draw_list[i];
        glUniformMatrix4fv(loc_model, 1, GL_FALSE, d.transform);
        for(unsigned t = 0; t < sizeof(d.textures) / sizeof(d.textures[0]); ++t) {
            glActiveTexture(GL_TEXTURE0 + t);
            glBindTexture(GL_TEXTURE_2D, d.textures[t]);
        }
        glBindVertexArray(d.vao);
        glDrawElements(GL_TRIANGLES, d.index_count, GL_UNSIGNED_INT, (GLvoid*)d.offset);
    }
}

}

#endif
