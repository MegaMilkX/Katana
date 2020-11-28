#include "gl_text_buffer.hpp"

#include "../../../render/vertex_format.hpp"

void glTextBuffer::rebuild(Font* font, const std::string& str, int face_height, TEXT_ALIGN alignment, gfxm::vec2 bb_size) {
    if(!font || str.empty()) {
        return;
    }

    if(!vao) {
        glGenBuffers(1, &vbuf);
        glGenBuffers(1, &uvbuf);
        glGenBuffers(1, &uvlookupbuf);

        glGenVertexArrays(1, &vao);
    }

    std::vector<gfxm::vec3> vertices;
    std::vector<gfxm::vec2> uv;
    std::vector<float>      uv_lookup_indices;
    float horiAdvance = .0f;
    float lineOffset = font->getLineHeight(face_height);
    gfxm::vec2 bb_min;
    gfxm::vec2 bb_max;

    int current_line_first_vertex = 0;
    int current_line_vertex_count = 0;
    float alignment_mul = .0f;
    if(alignment == TEXT_ALIGN_CENTER) {
        alignment_mul = 0.5f;
    } else if(alignment == TEXT_ALIGN_RIGHT) {
        alignment_mul = 1.0f;
    }
    float max_line_width = .0f;
    for(int i = 0; i < str.size(); ++i) {
        if(str[i] == '\n') {
            max_line_width = gfxm::_max(max_line_width, horiAdvance);
            for(int j = 0; j < current_line_vertex_count; ++j) {
                vertices[j + current_line_first_vertex].x -= (int)(horiAdvance * alignment_mul);
            }
            current_line_first_vertex = vertices.size();
            current_line_vertex_count = 0;

            lineOffset += font->getLineHeight(face_height);
            horiAdvance = .0f;
            continue;
        }
        const auto& g = font->getGlyph(str[i], face_height);
        float y_ofs = g.height - g.bearingY;
        float x_ofs = g.bearingX;
        
        gfxm::vec3 sw(horiAdvance + x_ofs,           0 - y_ofs - lineOffset,        0);
        gfxm::vec3 ne(horiAdvance + g.width + x_ofs, g.height - y_ofs - lineOffset, 0);
        
        bb_min = gfxm::vec2(gfxm::_min(bb_min.x, sw.x), gfxm::_min(bb_min.y, sw.y));
        bb_max = gfxm::vec2(gfxm::_max(bb_max.x, ne.x), gfxm::_max(bb_max.y, ne.y));

        vertices.push_back(gfxm::vec3(sw.x, sw.y, 0));
        vertices.push_back(gfxm::vec3(ne.x, sw.y, 0));
        vertices.push_back(gfxm::vec3(sw.x, ne.y, 0));
        vertices.push_back(gfxm::vec3(ne.x, sw.y, 0));
        vertices.push_back(gfxm::vec3(ne.x, ne.y, 0));
        vertices.push_back(gfxm::vec3(sw.x, ne.y, 0));
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

        current_line_vertex_count += 6;
    }
    vertex_count = vertices.size();
    int vertex_stride = sizeof(float) * 3;

    max_line_width = gfxm::_max(max_line_width, horiAdvance);
    max_line_width = gfxm::_max(max_line_width, bb_size.x);
    for(int j = 0; j < current_line_vertex_count; ++j) {
        vertices[j + current_line_first_vertex].x -= (int)(horiAdvance * alignment_mul);
    }
    current_line_first_vertex = vertices.size();
    current_line_vertex_count = 0;

    lineOffset += ceil(font->getLineHeight(face_height) * 0.5f); // Plus half line height for better centering

    this->bb_size.x  = max_line_width;
    this->bb_size.y  = lineOffset;

    for(auto& v : vertices) {
        //v.y += lineOffset;
        v.x += ceil(max_line_width * alignment_mul);
    }

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbuf);
    glEnableVertexAttribArray(VFMT::ENUM_TEXT::Position);
    glVertexAttribPointer(
        VFMT::ENUM_TEXT::Position, 3, GL_FLOAT, GL_FALSE,
        vertex_stride, 0
    );
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(gfxm::vec3), vertices.data(), GL_STREAM_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, uvbuf);
    glEnableVertexAttribArray(VFMT::ENUM_TEXT::UV);
    glVertexAttribPointer(
        VFMT::ENUM_TEXT::UV, 2, GL_FLOAT, GL_FALSE,
        sizeof(float) * 2, 0
    );
    glBufferData(GL_ARRAY_BUFFER, uv.size() * sizeof(gfxm::vec2), uv.data(), GL_STREAM_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, uvlookupbuf);
    glEnableVertexAttribArray(VFMT::ENUM_TEXT::TextUVLookup);
    glVertexAttribPointer(
        VFMT::ENUM_TEXT::TextUVLookup, 1, GL_FLOAT, GL_FALSE, 
        sizeof(float), 0
    );
    glBufferData(GL_ARRAY_BUFFER, uv_lookup_indices.size() * sizeof(float), uv_lookup_indices.data(), GL_STREAM_DRAW);

    glBindVertexArray(0);
}