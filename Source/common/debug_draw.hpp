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

    enum DEPTH {
        DEPTH_DISABLE,
        DEPTH_ENABLE
    };

    static DebugDraw* getInstance() {
        static DebugDraw* instance = new DebugDraw();
        return instance;
    }

    void init();
    void cleanup();

    void line(const gfxm::vec3& from, const gfxm::vec3& to, const gfxm::vec3& color, DEPTH depth_mode = DEPTH_ENABLE);

    void gridxy(const gfxm::vec3& from, const gfxm::vec3& to, float step, const gfxm::vec3& color);
    void gridxz(const gfxm::vec3& from, const gfxm::vec3& to, float step, const gfxm::vec3& color);
    void grid3d(const gfxm::vec3& from, const gfxm::vec3& to, float step, const gfxm::vec3& color);

    void aabb(const gfxm::aabb& aabb_, const gfxm::vec3& color);
    void aabb_corners(const gfxm::aabb& aabb_, const gfxm::vec3& color);

    void point(const gfxm::vec3& pt, const gfxm::vec3& color);

    void circle(const gfxm::vec3& center, float radius, const gfxm::vec3& color, const gfxm::mat3& transform = gfxm::mat3(1.0f));

    void frustum(const gfxm::mat4& proj, const gfxm::mat4& view, float znear, float zfar, const gfxm::vec3& color);

    void draw(const gfxm::mat4& proj, const gfxm::mat4& view);

    void clear();

private:
    std::vector<Vertex> line_buf;
    std::vector<Vertex> line_buf_no_depth;
    GLuint vao_handle = 0;
    GLuint vbuf = 0;
    gl::ShaderProgram* line_prog = 0;

    void draw(const gfxm::mat4& proj, const gfxm::mat4& view, const std::vector<Vertex> lines);

};

#endif
