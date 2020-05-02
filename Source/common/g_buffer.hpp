#ifndef G_BUFFER_HPP
#define G_BUFFER_HPP

#include "util/log.hpp"
#include "gl/frame_buffer.hpp"

class GBuffer {
public:
    enum ATTACHMENT {
        ALBEDO = 0,
        NORMAL,
        METALLIC,
        ROUGHNESS,
        LIGHTNESS
    };

    enum ATTACHMENT_BIT {
        ALBEDO_BIT = 1,
        NORMAL_BIT = 2,
        METALLIC_BIT = 4,
        ROUGHNESS_BIT = 8,
        LIGHTNESS_BIT = 16
    };

    GBuffer() {
        fb.addBuffer(ALBEDO, GL_RGB, GL_UNSIGNED_BYTE);    // albedo
        fb.addBuffer(NORMAL, GL_RGB16F, GL_FLOAT);         // normal
        fb.addBuffer(METALLIC, GL_RED, GL_UNSIGNED_BYTE);    // metallic
        fb.addBuffer(ROUGHNESS, GL_RED, GL_UNSIGNED_BYTE);    // roughness
        fb.addBuffer(LIGHTNESS, GL_RGBA32F, GL_FLOAT);    // lightness
    }
    void resize(unsigned width, unsigned height) {
        fb.reinitBuffers(width, height);
    }

    // Bind first!
    void enableAttachments(uint64_t mask, bool depth) {
        fb.enableAttachments(mask, depth);
    }

    GLsizei getWidth() {
        return fb.getWidth();
    }
    GLsizei getHeight() {
        return fb.getHeight();
    }

    GLuint getAlbedoTexture() {
        return fb.getTextureId(0);
    }
    GLuint getNormalTexture() {
        return fb.getTextureId(1);
    }
    GLuint getMetallicTexture() {
        return fb.getTextureId(2);
    }
    GLuint getRoughnessTexture() {
        return fb.getTextureId(3);
    }
    GLuint getLightnessTexture() {
        return fb.getTextureId(4);
    }
    GLuint getDepthTexture() {
        return fb.getDepthTextureId();
    }

    GLuint getGlFramebuffer() {
        return fb.getId();
    }
    void bind() {
        fb.bind();
    }
    void bind(uint64_t attachment_mask, bool depth_attachment) {
        fb.bind(attachment_mask, depth_attachment);
    }
private:
    gl::FrameBuffer fb;    
};

#endif
