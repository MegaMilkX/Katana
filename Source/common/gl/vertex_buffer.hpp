#ifndef VERTEX_BUFFER_HPP
#define VERTEX_BUFFER_HPP

#include "glextutil.h"
#include "error.hpp"

namespace gl {

class VertexAttribBuffer {
public:
    VertexAttribBuffer() {
        glGenBuffers(1, &id);
    }
    ~VertexAttribBuffer() {
        glDeleteBuffers(1, &id);
    }
    void data(void* data, size_t sz) {
        glBindBuffer(GL_ARRAY_BUFFER, id);
        GL_LOG_ERROR("glBindBuffer");
        glBufferData(GL_ARRAY_BUFFER, sz, data, GL_STATIC_DRAW);
        GL_LOG_ERROR("glBufferData");
        size = sz;
    }
    void enableAttrib(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride) {
        glBindBuffer(GL_ARRAY_BUFFER, id);
        GL_LOG_ERROR("glBindBuffer");
        glEnableVertexAttribArray(index);
        GL_LOG_ERROR("glEnableVertexAttribArray");
        glVertexAttribPointer(index, size, type, normalized, stride, 0);
        GL_LOG_ERROR("glVertexAttribPointer");
    }
    void bind() {
        glBindBuffer(GL_ARRAY_BUFFER, id);
        GL_LOG_ERROR("glBindBuffer");
    }

    size_t getDataSize() {
        return size;
    }
    bool copyData(void* dest, size_t sz) {
        if(!sz) {
            return false;
        }

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_COPY_READ_BUFFER, id);
        glGetBufferSubData(GL_COPY_READ_BUFFER, 0, sz, dest);
        glBindBuffer(GL_COPY_READ_BUFFER, 0);
        
        return true;
    }
private:
    GLuint id;
    size_t size = 0;

    VertexAttribBuffer(const VertexAttribBuffer &) {}
    VertexAttribBuffer& operator=(const VertexAttribBuffer &) {}
};

class IndexBuffer {
public:
    IndexBuffer() {
        glGenBuffers(1, &id);
    }
    ~IndexBuffer() {
        glDeleteBuffers(1, &id);
    }
    void data(void* data, size_t sz) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
        GL_LOG_ERROR("glBindBuffer");
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sz, data, GL_STATIC_DRAW);
        GL_LOG_ERROR("glBufferData");
        size = sz;
    }
    void bind() {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
        GL_LOG_ERROR("glBindBuffer");
    }

    size_t getDataSize() {
        return size;
    }
    bool copyData(void* dest, size_t sz) {
        if(!size) {
            return false;
        }

        glBindVertexArray(0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindBuffer(GL_COPY_READ_BUFFER, id);
        glGetBufferSubData(GL_COPY_READ_BUFFER, 0, sz, dest);
        glBindBuffer(GL_COPY_READ_BUFFER, 0);
        
        return true;
    }
private:
    GLuint id;
    size_t size = 0;

    IndexBuffer(const IndexBuffer &) {}
    IndexBuffer& operator=(const IndexBuffer &) {}
};

}

#endif
