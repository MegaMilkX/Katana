#ifndef DEBUG_DRAW_HPP
#define DEBUG_DRAW_HPP

#include "gfxm.hpp"

#include "gl/shader_program.h"
#include "shader_factory.hpp"

class DebugDraw {
public:
    struct Vertex {
        gfxm::vec3 position;
        gfxm::vec3 color;
    };

    static DebugDraw* getInstance() {
        static DebugDraw* instance = new DebugDraw();
        return instance;
    }

    void init();
    void cleanup();

    void line(const gfxm::vec3& from, const gfxm::vec3& to, const gfxm::vec3& color);

    void gridxy(const gfxm::vec3& from, const gfxm::vec3& to, float step, const gfxm::vec3& color);
    void gridxz(const gfxm::vec3& from, const gfxm::vec3& to, float step, const gfxm::vec3& color);
    void grid3d(const gfxm::vec3& from, const gfxm::vec3& to, float step, const gfxm::vec3& color);

    void aabb(const gfxm::aabb& aabb_, const gfxm::vec3& color);

    void draw(const gfxm::mat4& proj, const gfxm::mat4& view);

    void clear();

private:

    std::vector<Vertex> line_buf;
    GLuint vao_handle = 0;
    GLuint vbuf = 0;
    gl::ShaderProgram* line_prog = 0;
};

#endif
