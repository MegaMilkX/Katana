#include "render_gui.hpp"

#include "../../resource/font.hpp"

#include "../../gl/glextutil.h"

static GLuint createQuadVao() {
    static const std::vector<float> vertices = {
        0.0f,  0.0f, 0.0f, 0.0f, 1.0f, 
        1.0f,  0.0f, 0.0f, 1.0f, 1.0f,
        0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
        1.0f,  0.0f, 0.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 0.0f, 1.0f, 0.0f
    };

    GLuint vao_handle = 0;
    GLuint vbuf;
    glGenBuffers(1, &vbuf);

    glGenVertexArrays(1, &vao_handle);
    glBindVertexArray(vao_handle);
    glBindBuffer(GL_ARRAY_BUFFER, vbuf);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, 
        sizeof(float) * 5, 0
    );
    glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE, 
        sizeof(float) * 5, (void*)(sizeof(float) * 3)
    );

    glBindBuffer(GL_ARRAY_BUFFER, vbuf);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), (void*)vertices.data(), GL_STREAM_DRAW);
    
    return vao_handle;
}

void draw2d(const DrawList2d& dl, float screenW, float screenH) {
    static gl::ShaderProgram* sh_quad = shaderLoader().loadShaderProgram(
        "shaders/quad2d.glsl", false, VFMT::QUAD_2D::getVertexDesc()
    );
    static gl::ShaderProgram* sh_text = shaderLoader().loadShaderProgram(
        "shaders/text.glsl", false, VFMT::TEXT::getVertexDesc()
    );

    // TODO: Clean up buffer
    static GLuint vao = createQuadVao();

    //glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    glEnable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, screenW, screenH);

    gfxm::mat4 proj = gfxm::ortho(0.0f, screenW, 0.0f, screenH, -1000.0f, 1000.0f);

    for(size_t i = 0; i < dl.count; ++i) {
        auto& cmd = dl.array[i];

        glScissor(cmd.clip_rect.min.x, screenH + cmd.clip_rect.min.y - cmd.clip_rect.max.y, cmd.clip_rect.max.x, cmd.clip_rect.max.y);
        if(cmd.type == DRAW_CMD_QUAD) {
            sh_quad->use();
            glUniformMatrix4fv(sh_quad->getUniform("mat_proj"), 1, GL_FALSE, (float*)&proj);
            glBindVertexArray(vao);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cmd.quad.texture);
            glUniformMatrix4fv(sh_quad->getUniform("mat_model"), 1, GL_FALSE, (float*)&cmd.transform);
            glUniform4fv(sh_quad->getUniform("color"), 1, (float*)&cmd.quad.color);
            gfxm::vec2 quad_size = gfxm::vec2(cmd.quad.width, cmd.quad.height);
            glUniform2fv(sh_quad->getUniform("quad_size"), 1, (float*)&quad_size);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        } else if(cmd.type == DRAW_CMD_TEXT) {
            sh_text->use();
            glBindVertexArray(cmd.text.vao);
            glUniformMatrix4fv(sh_text->getUniform("mat_proj"), 1, GL_FALSE, (float*)&proj);
            glUniformMatrix4fv(sh_text->getUniform("mat_model"), 1, GL_FALSE, (float*)&cmd.transform);
            glUniform1i(sh_text->getUniform("lookupTextureWidth"), cmd.text.lookup_texture_width);
            
            GLuint texture = cmd.text.tex_atlas;
            GLuint lookupTexture = cmd.text.tex_lookup;
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture);
            glActiveTexture(GL_TEXTURE0 + 1);
            glBindTexture(GL_TEXTURE_2D, lookupTexture);

            glDrawArrays(GL_TRIANGLES, 0, cmd.text.vertex_count);
        } else if(cmd.type == DRAW_CMD_LINE) {
            static gl::ShaderProgram* sh = shaderLoader().loadShaderProgram(
                "shaders/line.glsl", false, VFMT::LINE::getVertexDesc()
            );
            sh->use();
            gfxm::mat4 model(1.0f);
            gfxm::mat4 view(1.0f);
            glUniformMatrix4fv(sh->getUniform("mat_proj"), 1, GL_FALSE, (float*)&proj);
            glUniformMatrix4fv(sh->getUniform("mat_model"), 1, GL_FALSE, (float*)&model);
            glUniformMatrix4fv(sh->getUniform("mat_view"), 1, GL_FALSE, (float*)&view);
            glUniform4fv(sh->getUniform("color"), 1, (float*)&cmd.line.color);

            glBindVertexArray(cmd.line.vao);
            glDrawArrays(GL_LINES, 0, cmd.line.vertex_count);
        }
    }
}