#ifndef FRAME_BUFFER_HPP
#define FRAME_BUFFER_HPP

#include "glextutil.h"
#include <vector>
#include "../util/log.hpp"

namespace gl {

class FrameBuffer {
public:
    struct BufferDesc {
        GLuint id = 0;
        GLint internalFormat;
        GLenum type;
    };

    FrameBuffer()
    : fbo(0), depth(0) {
        init();
    }
    ~FrameBuffer() {
        cleanup();
    }
    
    void pushBuffer(GLint internalFormat, GLenum type) {
        buffers.emplace_back(BufferDesc{0, internalFormat, type});
    }

    void reinitBuffers(unsigned width, unsigned height) {
        if(width == this->width && height == this->height) {
            return;
        }
        this->width = width;
        this->height = height;

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        int i = 0;
        for(auto& buf : buffers) {
            reinitBuffer(buf, width, height, i);
            ++i;
        }
        if(depth) glDeleteTextures(1, &depth);
        glGenTextures(1, &depth);
        glBindTexture(GL_TEXTURE_2D, depth);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
            (GLsizei)width, (GLsizei)height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth, 0);

        std::vector<GLenum> a;
        for(size_t i = 0; i < buffers.size(); ++i) {
            a.emplace_back(GL_COLOR_ATTACHMENT0 + i);
        }
        glDrawBuffers(buffers.size(), a.data());
        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            LOG_ERR("Framebuffer is incomplete");
        }
    }

    GLuint getId() const {
        return fbo;
    }

    void bind() {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    }

    GLuint getTextureId(int buf) {
        return buffers[buf].id;
    }
    GLuint getDepthTextureId() {
        return depth;
    }

    unsigned getWidth() const { return width; }
    unsigned getHeight() const { return height; }

private:
    FrameBuffer(const FrameBuffer &) {}
    FrameBuffer& operator=(const FrameBuffer &) {}

    void init() {
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    }
    void cleanup() {
        glDeleteFramebuffers(1, &fbo);
        if(depth) glDeleteTextures(1, &depth);
        for(auto& b : buffers) {
            if(b.id) glDeleteTextures(1, &b.id);
        }
    }
    void reinitBuffer(BufferDesc& desc, unsigned width, unsigned height, int attachment_num) {
        if(desc.id) glDeleteTextures(1, &desc.id);
        glGenTextures(1, &desc.id);
        glBindTexture(GL_TEXTURE_2D, desc.id);
        glTexImage2D(
            GL_TEXTURE_2D, 0, desc.internalFormat,
            (GLsizei)width, (GLsizei)height, 0, GL_RGB, desc.type, 0
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachment_num, desc.id, 0);
    }

    GLuint fbo;
    std::vector<BufferDesc> buffers;
    GLuint depth;
    unsigned width, height;
};

}

#endif
