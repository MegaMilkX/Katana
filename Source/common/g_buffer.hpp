#ifndef G_BUFFER_HPP
#define G_BUFFER_HPP

#include "util/log.hpp"
#include "gl/frame_buffer.hpp"

class GBuffer {
public:
    enum ATTACHMENT {
        ALBEDO,
        NORMAL,
        METALLIC,
        ROUGHNESS
    };

    GBuffer() {
        fb.pushBuffer(GL_RGB, GL_UNSIGNED_BYTE); // albedo
        fb.pushBuffer(GL_RGB16F, GL_FLOAT); // normal
        fb.pushBuffer(GL_RED, GL_UNSIGNED_BYTE); // metallic
        fb.pushBuffer(GL_RED, GL_UNSIGNED_BYTE); // roughness
    }
    void resize(unsigned width, unsigned height) {
        fb.reinitBuffers(width, height);
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
    GLuint getDepthTexture() {
        return fb.getDepthTextureId();
    }

    GLuint getGlFramebuffer() {
        return fb.getId();
    }
    void bind() {
        fb.bind();
    }
private:
    gl::FrameBuffer fb;    
};

#endif
