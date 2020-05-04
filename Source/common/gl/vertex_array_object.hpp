#ifndef GL_VERTEX_ARRAY_OBJECT_HPP
#define GL_VERTEX_ARRAY_OBJECT_HPP

#include "glextutil.h"


namespace gl {


class VertexArrayObject {
    GLuint id = 0;
public:
    VertexArrayObject() {
        glGenVertexArrays(1, &id);
    }
    ~VertexArrayObject() {
        glDeleteVertexArrays(1, &id);
    }

    GLuint getId() const {
        return id;
    }

    void attach(GLuint buffer, GLuint index, GLint elem_count, GLenum elem_type, GLboolean normalized, int stride, int offset) {
        glBindVertexArray(id);
        glEnableVertexAttribArray((GLuint)index);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glVertexAttribPointer((GLuint)index, elem_count, elem_type, normalized, stride, (void*)offset);
        glBindVertexArray(0);
    }

    void attachIndexBuffer(GLuint buffer) {
        glBindVertexArray(id);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
        glBindVertexArray(0);
    }

    void detach(GLuint index) {
        glBindVertexArray(id);
        glDisableVertexAttribArray((GLuint)index);
        glBindVertexArray(0);
    }

    void detachIndexBuffer() {
        glBindVertexArray(id);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void bind() {
        glBindVertexArray(id);
    }
};


};


#endif
