#ifndef GL_TEXT_BUFFER_HPP
#define GL_TEXT_BUFFER_HPP

#include "../../../gl/glextutil.h"
#include "../../../resource/font.hpp"

enum TEXT_ALIGN {
    TEXT_ALIGN_LEFT,
    TEXT_ALIGN_CENTER,
    TEXT_ALIGN_RIGHT
};

class glTextBuffer {
public:
    GLuint vao = 0;
    GLuint vbuf = 0;
    GLuint uvbuf = 0;
    GLuint uvlookupbuf = 0;
    int vertex_count = 0;
    gfxm::ivec2 bb_size;

    ~glTextBuffer() {
        if(vao) {
            glDeleteVertexArrays(1, &vao);
            glDeleteBuffers(1, &vbuf);
            glDeleteBuffers(1, &uvbuf);
            glDeleteBuffers(1, &uvlookupbuf);
            vertex_count = 0;
        }
    }

    void rebuild(Font* font, const std::string& str, int face_height, TEXT_ALIGN alignment = TEXT_ALIGN_LEFT, gfxm::vec2 bb_size = gfxm::vec2(0,0));
};


#endif
