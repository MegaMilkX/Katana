#ifndef GL_BUFFER_HPP
#define GL_BUFFER_HPP

#include "glextutil.h"
#include "../util/threading/delegated_call.hpp"

namespace gl {

class Buffer {
    GLuint id = 0;
    GLenum usage = 0;
public:
    Buffer(GLenum usage, size_t size = 0)
    : usage(usage) {
        delegatedCall([this, usage, size](){ 
            glGenBuffers(1, &id);
            if(size > 0) {
                glBindBuffer(GL_ARRAY_BUFFER, id);
                glBufferData(GL_ARRAY_BUFFER, size, 0, usage);
            }
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        });
    }
    ~Buffer() {
        delegatedCall([this](){ glDeleteBuffers(1, &id); });
    }

    GLuint getId() const {
        return id;
    }

    void upload(void* data, size_t size, GLenum new_usage) {
        delegatedCall([this, data, size, new_usage](){ 
            glBindBuffer(GL_ARRAY_BUFFER, id);
            glBufferData(GL_ARRAY_BUFFER, size, data, usage);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        });
    }

    void upload(void* data, size_t size) {
        upload(data, size, usage);
    }

    void bind(GLenum target) {
        glBindBuffer(target, id);
    }

    void bindBase(GLenum target, int index) {
        glBindBufferBase(target, index, id);
    }

};

}


#endif
