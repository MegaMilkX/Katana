#ifndef RENDER_VIEWPORT_HPP
#define RENDER_VIEWPORT_HPP

#include "../common/g_buffer.hpp"
#include "../common/gl/frame_buffer.hpp"
#include "../common/gl/indexed_mesh.hpp"

class RenderViewport {
public:
    RenderViewport() {

    }
    void init(unsigned w, unsigned h) {
        fb_fin.pushBuffer(GL_RGB, GL_UNSIGNED_BYTE);
        resize(w, h);
        width = w;
        height = h;
    }
    void resize(unsigned w, unsigned h) {
        if(width == w && height == h) return;
        g_buffer.resize(w, h);
        fb_fin.reinitBuffers(w, h);
        width = w;
        height = h;
    }

    unsigned getWidth() const {
        return width;
    }
    unsigned getHeight() const {
        return height;
    }

    GBuffer* getGBuffer() {
        return &g_buffer;
    }

    gl::FrameBuffer* getFinalBuffer() {
        return &fb_fin;
    }

    GLuint getFinalImage() {
        return fb_fin.getTextureId(0);
    }
private:
    unsigned width, height;
    GBuffer g_buffer;
    gl::FrameBuffer fb_fin;

    gl::IndexedMesh quad_mesh;
};

#endif
