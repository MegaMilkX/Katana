#ifndef UNIFORM_BUFFERS_HPP
#define UNIFORM_BUFFERS_HPP

#include <string>
#include <vector>
#include "glextutil.h"
#include "error.hpp"

namespace gl {

template<typename T, int INDEX>
class UniformBuffer {
public:
    UniformBuffer() {
        glGenBuffers(1, &id);
        glBindBuffer(GL_UNIFORM_BUFFER, id);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(T), 0, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glBindBufferBase(GL_UNIFORM_BUFFER, INDEX, id);
    }
    ~UniformBuffer() {
        glDeleteBuffers(1, &id);
    }

    void upload(const T& data) {
        glBindBuffer(GL_UNIFORM_BUFFER, id);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(T), &data, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void bind() {
        glBindBufferBase(GL_UNIFORM_BUFFER, INDEX, id);
    }
private:
    GLuint id = 0;
};

}

#endif
