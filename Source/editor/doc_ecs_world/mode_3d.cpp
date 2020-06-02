#include "mode_3d.hpp"


static GLuint createQuadVao() {
    static const std::vector<float> vertices = {
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 
        1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f
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
        "shaders/quad2d.glsl", false, VERTEX_FMT::QUAD_2D::getVertexDesc()
    );

    // TODO: Clean up buffer
    static GLuint vao = createQuadVao();

    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, screenW, screenH);

    glActiveTexture(GL_TEXTURE0);

    sh_quad->use();
    float aspect = screenW / screenH;
    float w;
    float h;
    if(screenW > screenH) {
        w = 1280.0f;
        h = 1280.0f / aspect;
    } else {
        w = 1280.0f * aspect;
        h = 1280.0f;
    }
    gfxm::mat4 proj = gfxm::ortho(0.0f, w, 0.0f, h, -1000.0f, 1000.0f);
    glUniformMatrix4fv(sh_quad->getUniform("mat_proj"), 1, GL_FALSE, (float*)&proj);

    glBindVertexArray(vao);
    for(size_t i = 0; i < dl.quad_count; ++i) {
        glBindTexture(GL_TEXTURE_2D, dl.quads[i].texture);
        glUniformMatrix4fv(sh_quad->getUniform("mat_model"), 1, GL_FALSE, (float*)&dl.quads[i].transform);
        glUniform4fv(sh_quad->getUniform("color"), 1, (float*)&dl.quads[i].color);
        gfxm::vec2 quad_size = gfxm::vec2(dl.quads[i].width, dl.quads[i].height);
        glUniform2fv(sh_quad->getUniform("quad_size"), 1, (float*)&quad_size);
        glUniform2fv(sh_quad->getUniform("origin"), 1, (float*)&dl.quads[i].origin);

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
}


void drawText(Font* fnt, const std::string& str, float screenW, float screenH, float x, float y) {
    std::vector<gfxm::vec3> vertices;
    std::vector<gfxm::vec2> uv;
    std::vector<float>      uv_lookup_indices;
    float horiAdvance = .0f;
    float lineOffset = fnt->getLineHeight();
    for(int i = 0; i < str.size(); ++i) {
        if(str[i] == '\n') {
            lineOffset += fnt->getLineHeight();
            horiAdvance = .0f;
            continue;
        }
        const auto& g = fnt->getGlyph(str[i]);
        float y_ofs = g.height - g.bearingY;
        float x_ofs = g.bearingX;
        vertices.push_back(gfxm::vec3(horiAdvance + x_ofs,           0 - y_ofs - lineOffset,        0));
        vertices.push_back(gfxm::vec3(horiAdvance + g.width + x_ofs, 0 - y_ofs - lineOffset,        0));
        vertices.push_back(gfxm::vec3(horiAdvance + x_ofs,           g.height - y_ofs - lineOffset, 0));
        vertices.push_back(gfxm::vec3(horiAdvance + g.width + x_ofs, 0 - y_ofs - lineOffset,        0));
        vertices.push_back(gfxm::vec3(horiAdvance + g.width + x_ofs, g.height - y_ofs - lineOffset, 0));
        vertices.push_back(gfxm::vec3(horiAdvance + x_ofs,           g.height - y_ofs - lineOffset, 0));
        horiAdvance += g.horiAdvance / 64.0f;

        uv.push_back(gfxm::vec2(0, 1));
        uv.push_back(gfxm::vec2(1, 1));
        uv.push_back(gfxm::vec2(0, 0));
        uv.push_back(gfxm::vec2(1, 1));
        uv.push_back(gfxm::vec2(1, 0));
        uv.push_back(gfxm::vec2(0, 0));

        uv_lookup_indices.push_back(g.cache_id * 4);
        uv_lookup_indices.push_back(g.cache_id * 4 + 1);
        uv_lookup_indices.push_back(g.cache_id * 4 + 3);
        uv_lookup_indices.push_back(g.cache_id * 4 + 1);
        uv_lookup_indices.push_back(g.cache_id * 4 + 2);
        uv_lookup_indices.push_back(g.cache_id * 4 + 3);
    }

    int vertex_stride = sizeof(float) * 3;
    
    GLuint vao = 0;
    GLuint vbuf = 0;
    GLuint uvbuf = 0;
    GLuint uvlookupbuf = 0;
    glGenBuffers(1, &vbuf);
    glGenBuffers(1, &uvbuf);
    glGenBuffers(1, &uvlookupbuf);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbuf);
    glEnableVertexAttribArray(VERTEX_FMT::ENUM_TEXT::Position);
    glVertexAttribPointer(
        VERTEX_FMT::ENUM_TEXT::Position, 3, GL_FLOAT, GL_FALSE,
        vertex_stride, 0
    );
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(gfxm::vec3), vertices.data(), GL_STREAM_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, uvbuf);
    glEnableVertexAttribArray(VERTEX_FMT::ENUM_TEXT::UV);
    glVertexAttribPointer(
        VERTEX_FMT::ENUM_TEXT::UV, 2, GL_FLOAT, GL_FALSE,
        sizeof(float) * 2, 0
    );
    glBufferData(GL_ARRAY_BUFFER, uv.size() * sizeof(gfxm::vec2), uv.data(), GL_STREAM_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, uvlookupbuf);
    glEnableVertexAttribArray(VERTEX_FMT::ENUM_TEXT::TextUVLookup);
    glVertexAttribPointer(
        VERTEX_FMT::ENUM_TEXT::TextUVLookup, 1, GL_FLOAT, GL_FALSE, 
        sizeof(float), 0
    );
    glBufferData(GL_ARRAY_BUFFER, uv_lookup_indices.size() * sizeof(float), uv_lookup_indices.data(), GL_STREAM_DRAW);

    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, screenW, screenH);
    static gl::ShaderProgram* sh = shaderLoader().loadShaderProgram("shaders/text.glsl", false, VERTEX_FMT::TEXT::getVertexDesc());
    sh->use();
    gfxm::mat4 proj = gfxm::ortho(0.0f, screenW, 0.0f, screenH, -1.0f, 1.0f);
    glUniformMatrix4fv(sh->getUniform("mat_proj"), 1, GL_FALSE, (float*)&proj);
    gfxm::mat4 model = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(x, y, 0));
    glUniformMatrix4fv(sh->getUniform("mat_model"), 1, GL_FALSE, (float*)&model);
    glUniform1i(sh->getUniform("lookupTextureWidth"), fnt->getLookupTextureWidth());
    
    GLuint texture = fnt->getAtlasTexture();
    GLuint lookupTexture = fnt->getGlyphLookupTexture();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, lookupTexture);

    glDrawArrays(GL_TRIANGLES, 0, vertices.size());

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbuf);
    glDeleteBuffers(1, &uvbuf);
    glDeleteBuffers(1, &uvlookupbuf);
}