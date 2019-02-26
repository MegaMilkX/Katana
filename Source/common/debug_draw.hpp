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

    void init() {
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
    void cleanup() {
        glDeleteBuffers(1, &vbuf);
        glDeleteVertexArrays(1, &vao_handle);
    }

    void line(const gfxm::vec3& from, const gfxm::vec3& to, const gfxm::vec3& color) {
        Vertex a{from, color};
        Vertex b{to, color};

        line_buf.emplace_back(a);
        line_buf.emplace_back(b);
    }

    void draw(const gfxm::mat4& proj, const gfxm::mat4& view) {
        if(line_buf.empty()) return;

        //glDisable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vbuf);
        glBufferData(GL_ARRAY_BUFFER, line_buf.size() * sizeof(Vertex), line_buf.data(), GL_STREAM_DRAW);

        glBindVertexArray(vao_handle);

        line_prog->use();
        glUniformMatrix4fv(line_prog->getUniform("mat_projection"), 1, GL_FALSE, (float*)&proj);
        glUniformMatrix4fv(line_prog->getUniform("mat_view"), 1, GL_FALSE, (float*)&view);

        glDrawArrays(GL_LINES, 0, line_buf.size());
    }

    void clear() {
        line_buf.clear();
    }

private:

    std::vector<Vertex> line_buf;
    GLuint vao_handle = 0;
    GLuint vbuf = 0;
    gl::ShaderProgram* line_prog = 0;
};

#endif
