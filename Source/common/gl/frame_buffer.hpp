#ifndef FRAME_BUFFER_HPP
#define FRAME_BUFFER_HPP

#include "glextutil.h"
#include <vector>
#include <assert.h>
#include "../util/log.hpp"

namespace gl {

class FrameBuffer {
public:
    struct BufferDesc {
        GLuint id = 0;
        int attachment_id = 0;
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

    // Bind first!
    void enableAttachments(uint64_t mask, bool depth_attachment) {
        for(int i = 0; i < buffers.size(); ++i) {
            if((1 << buffers[i].attachment_id) & mask) {
                glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + buffers[i].attachment_id, buffers[i].id, 0);
            } else {
                glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + buffers[i].attachment_id, 0, 0);
            }
        }
        if(depth_attachment) {
            glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth, 0);
        } else {
            glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,     0, 0);
        }
        int ret = glGetError();
        assert(ret == GL_NO_ERROR);

        // TODO: CHECK GL_MAX_DRAW_BUFFERS
        static GLenum draw_buffers[8] = {
            GL_NONE, GL_NONE, GL_NONE, GL_NONE, GL_NONE, GL_NONE, GL_NONE, GL_NONE
        };
        for (size_t i = 0; i < buffers.size(); ++i) {
            if ((1 << buffers[i].attachment_id) & mask) {
                draw_buffers[buffers[i].attachment_id] = GL_COLOR_ATTACHMENT0 + buffers[i].attachment_id;
            } else {
                draw_buffers[buffers[i].attachment_id] = GL_NONE;
            }
        }
        glDrawBuffers(8, draw_buffers);
        ret = glGetError();
        assert(ret == GL_NO_ERROR);

        ret = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if(ret != GL_FRAMEBUFFER_COMPLETE) {
            LOG_ERR("Framebuffer is incomplete");
            assert(false);
        }
    }
    
    void addBuffer(int attachment_id, GLint internalFormat, GLenum type) {
        buffers.emplace_back(BufferDesc{0, attachment_id, internalFormat, type});
    }

    void reinitBuffers(unsigned width, unsigned height) {
        if(width == this->width && height == this->height) {
            return;
        }
        this->width = width;
        this->height = height;

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        for(auto& buf : buffers) {
            reinitBuffer(buf, width, height, buf.attachment_id);
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

    void bind(uint64_t attachment_mask, bool depth_attachment) {
        bind();
        enableAttachments(attachment_mask, depth_attachment);
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
        GLenum err = glGetError();
        if(err) {
            LOG_WARN("Framebuffer layer error, glTexImage2D: " << err);
        }
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
