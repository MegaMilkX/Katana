#include "debug_draw.hpp"

void DebugDraw::init() {
    glGenVertexArrays(1, &vao_handle);
    glGenBuffers(1, &vbuf);

    glBindVertexArray(vao_handle);
    glBindBuffer(GL_ARRAY_BUFFER, vbuf);
    glEnableVertexAttribArray(gl::POSITION); // VERTEX
    glEnableVertexAttribArray(gl::COLOR_RGBA); // COLOR
    glVertexAttribPointer(gl::POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
    glVertexAttribPointer(gl::COLOR_RGBA, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)sizeof(gfxm::vec3));

    line_prog = ShaderFactory::getOrCreate(
        "bullet_debug_line",
        #include "shaders/v_debug_line.glsl"
        ,
        #include "shaders/f_debug_line.glsl"
    );
}
void DebugDraw::cleanup() {
    glDeleteBuffers(1, &vbuf);
    glDeleteVertexArrays(1, &vao_handle);
}

void DebugDraw::line(const gfxm::vec3& from, const gfxm::vec3& to, const gfxm::vec3& color) {
    Vertex a{from, color};
    Vertex b{to, color};

    line_buf.emplace_back(a);
    line_buf.emplace_back(b);
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

void DebugDraw::draw(const gfxm::mat4& proj, const gfxm::mat4& view) {
    if(line_buf.empty()) return;

    //glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vbuf);
    glBufferData(GL_ARRAY_BUFFER, line_buf.size() * sizeof(Vertex), line_buf.data(), GL_DYNAMIC_DRAW);

    glBindVertexArray(vao_handle);

    line_prog->use();
    glUniformMatrix4fv(line_prog->getUniform("mat_projection"), 1, GL_FALSE, (float*)&proj);
    glUniformMatrix4fv(line_prog->getUniform("mat_view"), 1, GL_FALSE, (float*)&view);

    glDrawArrays(GL_LINES, 0, line_buf.size());
}

void DebugDraw::clear() {
    line_buf.clear();
}