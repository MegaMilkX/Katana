#ifndef DEBUG_DRAW_HPP
#define DEBUG_DRAW_HPP

#include "gfxm.hpp"

#include "gl/shader_program.h"
#include "gl/indexed_mesh.hpp"
#include "shader_factory.hpp"

#include <list>

class DebugDraw {
public:
    struct Vertex {
        gfxm::vec3 position;
        gfxm::vec3 color;
    };
    struct Model {
        gl::IndexedMesh* mesh;
        gfxm::mat4 transform;
        gfxm::vec4 color;
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

    void sphere(const gfxm::vec3& pt, float radius, const gfxm::vec3& color);

    void circle(const gfxm::vec3& center, float radius, const gfxm::vec3& color, const gfxm::mat3& transform = gfxm::mat3(1.0f));

    void frustum(const gfxm::mat4& proj, const gfxm::mat4& view, float znear, float zfar, const gfxm::vec3& color);

    void mesh(gl::IndexedMesh* mesh, const gfxm::mat4& model, const gfxm::vec4& color = gfxm::vec4(0.4f, 1.0f, .5f, 0.5f));

    void draw(const gfxm::mat4& proj, const gfxm::mat4& view);

    void clear();

private:
    std::vector<Vertex> line_buf;
    std::vector<Vertex> line_buf_no_depth;
    std::list<Model>    meshes;
    GLuint vao_handle = 0;
    GLuint vbuf = 0;
    gl::ShaderProgram* line_prog = 0;
    gl::ShaderProgram* tri_prog = 0;

    void draw(const gfxm::mat4& proj, const gfxm::mat4& view, const std::vector<Vertex> lines);

};

#endif
