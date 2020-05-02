#include "debug_draw.hpp"

#include "render/shader_loader.hpp"

void DebugDraw::init() {
    glGenVertexArrays(1, &vao_handle);
    glGenBuffers(1, &vbuf);

    glBindVertexArray(vao_handle);
    glBindBuffer(GL_ARRAY_BUFFER, vbuf);
    glEnableVertexAttribArray(VERTEX_FMT::ENUM_GENERIC::Position); // VERTEX
    glEnableVertexAttribArray(VERTEX_FMT::ENUM_GENERIC::ColorRGBA); // COLOR
    glVertexAttribPointer(VERTEX_FMT::ENUM_GENERIC::Position, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
    glVertexAttribPointer(VERTEX_FMT::ENUM_GENERIC::ColorRGBA, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)sizeof(gfxm::vec3));

    line_prog = shaderLoader().loadShaderProgram("shaders/debug_draw/bullet_debug_line.glsl");
    tri_prog = shaderLoader().loadShaderProgram("shaders/debug_draw/triangle.glsl");
}
void DebugDraw::cleanup() {
    glDeleteBuffers(1, &vbuf);
    glDeleteVertexArrays(1, &vao_handle);
}

void DebugDraw::line(const gfxm::vec3& from, const gfxm::vec3& to, const gfxm::vec3& color, DEPTH depth_mode) {
    Vertex a{from, color};
    Vertex b{to, color};

    if(depth_mode == DEPTH_ENABLE) {
        line_buf.emplace_back(a);
        line_buf.emplace_back(b);
    } else {
        line_buf_no_depth.emplace_back(a);
        line_buf_no_depth.emplace_back(b);
    }
}

void DebugDraw::gridxy(const gfxm::vec3& from, const gfxm::vec3& to, float step, const gfxm::vec3& color) {
    for(float x = from.x; x < to.x + step * 0.5f; x += step) {
        line(
            gfxm::vec3(x, from.y, .0f),
            gfxm::vec3(x, to.y, .0f),
            color
        );
    }
    for(float y = from.y; y < to.y + step * 0.5f; y += step) {
        line(
            gfxm::vec3(from.x, y, .0f),
            gfxm::vec3(to.x, y, .0f),
            color
        );
    }
}

void DebugDraw::gridxz(const gfxm::vec3& from, const gfxm::vec3& to, float step, const gfxm::vec3& color) {
    for(float x = from.x; x < to.x + step * 0.5f; x += step) {
        line(
            gfxm::vec3(x, .0f, from.z),
            gfxm::vec3(x, .0f, to.z),
            color
        );
    }
    for(float z = from.z; z < to.z + step * 0.5f; z += step) {
        line(
            gfxm::vec3(from.x, .0f, z),
            gfxm::vec3(to.x, .0f, z),
            color
        );
    }
}

void DebugDraw::grid3d(const gfxm::vec3& from, const gfxm::vec3& to, float step, const gfxm::vec3& color) {
    for(float x = from.x; x < to.x + step * 0.5f; x += step) {
        for(float y = from.y; y < to.y + step * 0.5f; y += step) {
            line(
                gfxm::vec3(x, y, from.z),
                gfxm::vec3(x, y, to.z),
                color
            );
        }
    }
    for(float x = from.x; x < to.x + step * 0.5f; x += step) {
        for(float z = from.z; z < to.z + step * 0.5f; z += step) {
            line(
                gfxm::vec3(x, from.y, z),
                gfxm::vec3(x, to.y, z),
                color
            );
        }
    }
    for(float y = from.y; y < to.y + step * 0.5f; y += step) {
        for(float z = from.z; z < to.z + step * 0.5f; z += step) {
            line(
                gfxm::vec3(from.x, y, z),
                gfxm::vec3(to.x, y, z),
                color
            );
        }
    }
}

void DebugDraw::aabb(const gfxm::aabb& aabb_, const gfxm::vec3& color) {
    line(gfxm::vec3(aabb_.from.x, aabb_.from.y, aabb_.from.z), gfxm::vec3(aabb_.to.x, aabb_.from.y, aabb_.from.z), color);
    line(gfxm::vec3(aabb_.from.x, aabb_.to.y, aabb_.from.z), gfxm::vec3(aabb_.to.x, aabb_.to.y, aabb_.from.z), color);
    line(gfxm::vec3(aabb_.from.x, aabb_.from.y, aabb_.to.z), gfxm::vec3(aabb_.to.x, aabb_.from.y, aabb_.to.z), color);
    line(gfxm::vec3(aabb_.from.x, aabb_.to.y, aabb_.to.z), gfxm::vec3(aabb_.to.x, aabb_.to.y, aabb_.to.z), color);

    line(gfxm::vec3(aabb_.from.x, aabb_.from.y, aabb_.from.z), gfxm::vec3(aabb_.from.x, aabb_.to.y, aabb_.from.z), color);
    line(gfxm::vec3(aabb_.to.x, aabb_.from.y, aabb_.from.z), gfxm::vec3(aabb_.to.x, aabb_.to.y, aabb_.from.z), color);
    line(gfxm::vec3(aabb_.from.x, aabb_.from.y, aabb_.to.z), gfxm::vec3(aabb_.from.x, aabb_.to.y, aabb_.to.z), color);
    line(gfxm::vec3(aabb_.to.x, aabb_.from.y, aabb_.to.z), gfxm::vec3(aabb_.to.x, aabb_.to.y, aabb_.to.z), color);

    line(gfxm::vec3(aabb_.from.x, aabb_.from.y, aabb_.from.z), gfxm::vec3(aabb_.from.x, aabb_.from.y, aabb_.to.z), color);
    line(gfxm::vec3(aabb_.to.x, aabb_.from.y, aabb_.from.z), gfxm::vec3(aabb_.to.x, aabb_.from.y, aabb_.to.z), color);
    line(gfxm::vec3(aabb_.from.x, aabb_.to.y, aabb_.from.z), gfxm::vec3(aabb_.from.x, aabb_.to.y, aabb_.to.z), color);
    line(gfxm::vec3(aabb_.to.x, aabb_.to.y, aabb_.from.z), gfxm::vec3(aabb_.to.x, aabb_.to.y, aabb_.to.z), color);
}

void DebugDraw::aabb_corners(const gfxm::aabb& aabb_, const gfxm::vec3& color) {
    gfxm::vec3 from = gfxm::vec3(aabb_.from.x, aabb_.from.y, aabb_.from.z);
    gfxm::vec3 to = gfxm::vec3(aabb_.to.x, aabb_.from.y, aabb_.from.z);
    gfxm::vec3 diff = to - from;
    diff = gfxm::normalize(diff) * (gfxm::length(diff) / 3);
    line(from, from + diff, color);
}

void DebugDraw::point(const gfxm::vec3& pt, const gfxm::vec3& color) {
    line(gfxm::vec3(-0.5f,0,0) + pt, gfxm::vec3(0.5f,0,0) + pt, color);
    line(gfxm::vec3(0,-0.5f,0) + pt, gfxm::vec3(0,0.5f,0) + pt, color);
    line(gfxm::vec3(0,0,-0.5f) + pt, gfxm::vec3(0,0,0.5f) + pt, color);
}

void DebugDraw::sphere(const gfxm::vec3& pt, float radius, const gfxm::vec3& color) {
    gfxm::mat3 m = gfxm::to_mat3(gfxm::angle_axis(0, gfxm::vec3(0,1,0)));
    circle(pt, radius, color, m);
    m = gfxm::to_mat3(gfxm::angle_axis(gfxm::radian(90.0f), gfxm::vec3(1,0,0)));
    circle(pt, radius, color, m);
    m = gfxm::to_mat3(gfxm::angle_axis(gfxm::radian(90.0f), gfxm::vec3(0,0,1)));
    circle(pt, radius, color, m);
}

void DebugDraw::circle(const gfxm::vec3& center, float radius, const gfxm::vec3& color, const gfxm::mat3& transform) {
    const int reso = 12;
    for(int i = 1; i <= reso; ++i) {
        float v_a = ((i - 1) / (float)reso) * gfxm::pi * 2.0f;
        float v_b = (i / (float)reso) * gfxm::pi * 2.0f;
        gfxm::vec3 pt_a = gfxm::to_mat4(transform) * gfxm::vec4(sinf(v_a) * radius, 0, cosf(v_a) * radius, 0);
        gfxm::vec3 pt_b = gfxm::to_mat4(transform) * gfxm::vec4(sinf(v_b) * radius, 0, cosf(v_b) * radius, 0);
        line(center + pt_a, center + pt_b, color);
    }
}

void DebugDraw::frustum(const gfxm::mat4& proj, const gfxm::mat4& view, float znear, float zfar, const gfxm::vec3& color) {
    gfxm::vec3 points[] = {
        { -1, 1, 1 },
        { 1, 1, 1 },
        { 1, -1, 1 },
        { -1, -1, 1 },
        { -1, 1, -1 },
        { 1, 1, -1 },
        { 1, -1, -1 },
        { -1, -1, -1 }
    };

    for(size_t i = 0; i < sizeof(points) / sizeof(points[0]); ++i) {
        gfxm::vec4 p(points[i], 1.0f);
        p = gfxm::inverse(proj) * p;
        p.x /= p.w;
        p.y /= p.w;
        p.z /= p.w;
        p.w = 1.0f;
        p = gfxm::inverse(view) * p;
        points[i] = p;
    }

    line(points[0], points[1], color);
    line(points[1], points[2], color);
    line(points[2], points[3], color);
    line(points[3], points[0], color);

    line(points[4], points[5], color);
    line(points[5], points[6], color);
    line(points[6], points[7], color);
    line(points[7], points[4], color);

    line(points[0], points[4], color);
    line(points[1], points[5], color);
    line(points[2], points[6], color);
    line(points[3], points[7], color);
}

void DebugDraw::mesh(gl::IndexedMesh* mesh, const gfxm::mat4& model, const gfxm::vec4& color) {
    meshes.insert(meshes.end(), Model{mesh, model, color});
}

void DebugDraw::draw(const gfxm::mat4& proj, const gfxm::mat4& view) {
    if(line_buf.empty() && line_buf_no_depth.empty()) return;

    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL); // Important
    
    glEnable(GL_DEPTH_TEST);
    draw(proj, view, line_buf);

    glDisable(GL_DEPTH_TEST);
    draw(proj, view, line_buf_no_depth);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glEnable(GL_DEPTH_TEST);
    tri_prog->use();
    glUniformMatrix4fv(tri_prog->getUniform("mat_projection"), 1, GL_FALSE, (float*)&proj);
    glUniformMatrix4fv(tri_prog->getUniform("mat_view"), 1, GL_FALSE, (float*)&view);
    for(auto& m : meshes) {
        m.mesh->bind();
        glUniformMatrix4fv(tri_prog->getUniform("mat_model"), 1, GL_FALSE, (float*)&m.transform);
        glUniform4f(tri_prog->getUniform("u_color"), m.color.x, m.color.y, m.color.z, m.color.w);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDrawElements(GL_TRIANGLES, m.mesh->getIndexCount(), GL_UNSIGNED_INT, 0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawElements(GL_TRIANGLES, m.mesh->getIndexCount(), GL_UNSIGNED_INT, 0);
    }
}

void DebugDraw::clear() {
    line_buf.clear();
    line_buf_no_depth.clear();
    meshes.clear();
}


void DebugDraw::draw(const gfxm::mat4& proj, const gfxm::mat4& view, const std::vector<Vertex> lines) {
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vbuf);
    glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(Vertex), lines.data(), GL_DYNAMIC_DRAW);

    glBindVertexArray(vao_handle);

    line_prog->use();
    glUniformMatrix4fv(line_prog->getUniform("mat_projection"), 1, GL_FALSE, (float*)&proj);
    glUniformMatrix4fv(line_prog->getUniform("mat_view"), 1, GL_FALSE, (float*)&view);

    glDrawArrays(GL_LINES, 0, lines.size());
}