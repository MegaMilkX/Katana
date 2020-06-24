#include "mode_3d.hpp"

#include "../../common/render/mesh_data.hpp"


static struct {
    gfxm::rect  viewport;
    GIZMO_RECT_CONTROL_POINT cp             = GIZMO_RECT_NONE;
    int                      cp_flags       = GIZMO_RECT_CP_ALL;
    bool                     btn_pressed    = false;
    bool                     dragging       = false;
    gfxm::vec2               last_mouse;

    gfxm::mat4 transform;
    gfxm::vec3 origin;
    float rotation_prev = .0f;
    gfxm::vec3 nw;
    gfxm::vec3 se;
    gfxm::vec3 ne;
    gfxm::vec3 sw;
} gizmo_rect;
void gizmoRect2dViewport(float left, float right, float bottom, float top) {
    gizmo_rect.viewport = gfxm::rect(left, bottom, right, top);
}
bool gizmoRect2d(gfxm::mat4& t, const gfxm::mat4& local, gfxm::vec2& lcl_pos_offs, float& rotation, gfxm::vec2& origin_, gfxm::rect& rect, const gfxm::vec2& mouse, bool button_pressed, int cp_flags = GIZMO_RECT_CP_ALL) {
    gizmo_rect.transform = t;
    gizmo_rect.origin = gfxm::vec3(origin_.x, origin_.y, .0f);
    gizmo_rect.cp_flags = cp_flags;
    gizmo_rect.nw = gfxm::vec3(rect.min.x, rect.max.y, .0f);
    gizmo_rect.se = gfxm::vec3(rect.max.x, rect.min.y, .0f);
    gizmo_rect.ne = gfxm::vec3(gizmo_rect.se.x, gizmo_rect.nw.y, .0f);
    gizmo_rect.sw = gfxm::vec3(gizmo_rect.nw.x, gizmo_rect.se.y, .0f);

    gfxm::mat4 parent_transform = gfxm::inverse(local) * t;

    gfxm::vec3 mouse3(mouse.x, mouse.y, .0f);
    mouse3 = gfxm::inverse(t) * gfxm::vec4(mouse3, 1.0f);
    const float active_radius = 8.0f;
    const float active_radius12 = 12.0f;

    gfxm::vec3 north = gfxm::lerp(gizmo_rect.nw, gizmo_rect.ne, 0.5f);
    gfxm::vec3 south = gfxm::lerp(gizmo_rect.sw, gizmo_rect.se, 0.5f);
    gfxm::vec3 west = gfxm::lerp(gizmo_rect.nw, gizmo_rect.sw, 0.5f);
    gfxm::vec3 east = gfxm::lerp(gizmo_rect.ne, gizmo_rect.se, 0.5f);
    gfxm::vec3 origin 
        = gfxm::vec3(
            gfxm::lerp(gizmo_rect.nw.x, gizmo_rect.ne.x, origin_.x), 
            gfxm::lerp(gizmo_rect.nw.y, gizmo_rect.sw.y, origin_.y), 
            0
        );//gfxm::lerp(gizmo_rect.nw, gizmo_rect.se, 0.5f);

    bool just_pressed = !gizmo_rect.btn_pressed && button_pressed;
    bool just_released = gizmo_rect.btn_pressed && !button_pressed;

    gfxm::vec3 rotator_pos = gfxm::lerp(gizmo_rect.nw, gizmo_rect.ne, 0.5f) + gfxm::vec3(.0f, 20.0f, .0f);
    if(!gizmo_rect.btn_pressed) {
        if((cp_flags & GIZMO_RECT_ORIGIN) && gfxm::length(mouse3 - origin) < active_radius) {
            gizmo_rect.cp = GIZMO_RECT_ORIGIN;
        } else if((cp_flags & GIZMO_RECT_SE) && gfxm::length(mouse3 - gizmo_rect.se) < active_radius12) {
            gizmo_rect.cp = GIZMO_RECT_SE;
        } else if((cp_flags & GIZMO_RECT_NE) && gfxm::length(mouse3 - gizmo_rect.ne) < active_radius12) {
            gizmo_rect.cp = GIZMO_RECT_NE;
        } else if((cp_flags & GIZMO_RECT_SW) && gfxm::length(mouse3 - gizmo_rect.sw) < active_radius12) {
            gizmo_rect.cp = GIZMO_RECT_SW;
        } else if((cp_flags & GIZMO_RECT_NW) && gfxm::length(mouse3 - gizmo_rect.nw) < active_radius12) {
            gizmo_rect.cp = GIZMO_RECT_NW;
        } else if((cp_flags & GIZMO_RECT_SOUTH) && gfxm::length(mouse3 - south) < active_radius12) {
            gizmo_rect.cp = GIZMO_RECT_SOUTH;
        } else if((cp_flags & GIZMO_RECT_EAST) && gfxm::length(mouse3 - east) < active_radius12) {
            gizmo_rect.cp = GIZMO_RECT_EAST;
        } else if((cp_flags & GIZMO_RECT_NORTH) && gfxm::length(mouse3 - north) < active_radius12) {
            gizmo_rect.cp = GIZMO_RECT_NORTH;
        } else if((cp_flags & GIZMO_RECT_WEST) && gfxm::length(mouse3 - west) < active_radius12) {
            gizmo_rect.cp = GIZMO_RECT_WEST;
        } else if((cp_flags & GIZMO_RECT_BOX) && rect.min.x < mouse3.x && rect.max.x > mouse3.x && rect.min.y < mouse3.y && rect.max.y > mouse3.y) {
            gizmo_rect.cp = GIZMO_RECT_BOX;
        } else if((cp_flags & GIZMO_RECT_ROTATE) && gfxm::length(mouse3 - rotator_pos) < active_radius) {
            gizmo_rect.cp = GIZMO_RECT_ROTATE;
        } else {
            gizmo_rect.cp = GIZMO_RECT_NONE;
        }
    }

    gizmo_rect.btn_pressed = button_pressed;
    gfxm::rect delta;
    delta.min = gfxm::vec2(0,0);
    delta.max = gfxm::vec2(0,0);
    
    if(gizmo_rect.btn_pressed) {
        gfxm::vec3 last_mouse3 = gfxm::vec3(gizmo_rect.last_mouse, .0f);
        last_mouse3 = gfxm::inverse(t) * gfxm::vec4(last_mouse3, 1.0f);
        gfxm::vec3 offs = mouse3 - last_mouse3;
        gfxm::vec3 lcl_offs = gfxm::inverse(parent_transform) * t * gfxm::vec4(offs, .0f);

        if(gizmo_rect.cp == GIZMO_RECT_NE ||
            gizmo_rect.cp == GIZMO_RECT_EAST ||
            gizmo_rect.cp == GIZMO_RECT_SE
        ) {
            delta.max.x += offs.x;
        }
        if(gizmo_rect.cp == GIZMO_RECT_NW ||
            gizmo_rect.cp == GIZMO_RECT_WEST ||
            gizmo_rect.cp == GIZMO_RECT_SW
        ) {
            delta.min.x += offs.x;
        }
        if(gizmo_rect.cp == GIZMO_RECT_NW ||
            gizmo_rect.cp == GIZMO_RECT_NORTH ||
            gizmo_rect.cp == GIZMO_RECT_NE
        ) {
            delta.max.y += offs.y;
        }
        if(gizmo_rect.cp == GIZMO_RECT_SW ||
            gizmo_rect.cp == GIZMO_RECT_SOUTH ||
            gizmo_rect.cp == GIZMO_RECT_SE
        ) {
            delta.min.y += offs.y;
        }

        if(gizmo_rect.cp == GIZMO_RECT_BOX) {
            lcl_pos_offs = gfxm::vec2(lcl_offs.x, lcl_offs.y);
        }

        if(gizmo_rect.cp == GIZMO_RECT_ROTATE) {
            gfxm::vec3 O = t * gfxm::vec4(origin, 1.0f);
            gfxm::vec2 N = gfxm::normalize(gfxm::vec2(mouse.x, mouse.y) - gfxm::vec2(O.x, O.y));
            float theta = atan2(N.x, N.y);
            float deg = gfxm::degrees(theta);
            
            if(just_pressed) {
                gizmo_rect.rotation_prev = deg;
            } else {
                float prev = gizmo_rect.rotation_prev;
                float delta = deg - prev;

                rotation -= delta;

                gizmo_rect.rotation_prev = deg;
            }
        }

        if(gizmo_rect.cp == GIZMO_RECT_ORIGIN) {
            float noffs_x = offs.x / (gizmo_rect.ne.x - gizmo_rect.nw.x);
            float noffs_y = offs.y / (gizmo_rect.nw.y - gizmo_rect.sw.y);
            origin_.x += noffs_x;
            origin_.y -= noffs_y;

            lcl_pos_offs = gfxm::vec2(lcl_offs.x, lcl_offs.y);
        }

        rect.min += delta.min;
        rect.max += delta.max;
    }

    gizmo_rect.last_mouse = mouse;

    if(gizmo_rect.cp != GIZMO_RECT_NONE && (gizmo_rect.btn_pressed || button_pressed)) {
        return true;
    } else {
        return false;
    }
}
void gizmoRect2dDraw(float screenW, float screenH) {
    const gfxm::vec4 color(1,1,1,1);

    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(
        0, 0,
        abs(gizmo_rect.viewport.max.x - gizmo_rect.viewport.min.x), 
        abs(gizmo_rect.viewport.max.y - gizmo_rect.viewport.min.y)
    );

    gfxm::mat4 proj = gfxm::ortho(
        gizmo_rect.viewport.min.x, gizmo_rect.viewport.max.x, 
        gizmo_rect.viewport.min.y, gizmo_rect.viewport.max.y, 
        -1000.0f, 1000.0f
    );

    gfxm::vec4 col(0.7f, 0.5f, 0.6f, 1.0f);
    gfxm::vec4 col_hover(1.0f, 0.8f, 0.0f, 1.0f);

    MeshData<VERTEX_FMT::LINE> mesh;
    std::vector<gfxm::vec3> vertices = {
        gizmo_rect.nw, gizmo_rect.ne,
        gizmo_rect.ne, gizmo_rect.se,
        gizmo_rect.se, gizmo_rect.sw,
        gizmo_rect.sw, gizmo_rect.nw
    };
    const gfxm::vec4& c = gizmo_rect.cp == GIZMO_RECT_BOX ? col_hover : col;
    uint8_t r = 255 * c.x;
    uint8_t g = 255 * c.y;
    uint8_t b = 255 * c.z;
    uint8_t a = 255 * c.w;
    std::vector<unsigned char> colors = {
        r, g, b, a,   r, g, b, a,
        r, g, b, a,   r, g, b, a,
        r, g, b, a,   r, g, b, a,
        r, g, b, a,   r, g, b, a
    };

    std::function<void(const gfxm::vec3&, float, const gfxm::vec4&)> add_square_fn = 
    [&vertices, &colors](const gfxm::vec3& pos, float sz, const gfxm::vec4& col){
        float sz_half = sz * 0.5f;
        gfxm::vec3 nw = gfxm::vec3(pos.x - sz_half, pos.y + sz_half, .0f);
        gfxm::vec3 se = gfxm::vec3(pos.x + sz_half, pos.y - sz_half, .0f);
        gfxm::vec3 ne = gfxm::vec3(se.x, nw.y, .0f);
        gfxm::vec3 sw = gfxm::vec3(nw.x, se.y, .0f);
        std::vector<gfxm::vec3> v = {
            nw, ne,
            ne, se,
            se, sw,
            sw, nw
        };
        uint8_t r = 255 * col.x;
        uint8_t g = 255 * col.y;
        uint8_t b = 255 * col.z;
        uint8_t a = 255 * col.w;
        std::vector<unsigned char> c = {
            r, g, b, a,   r, g, b, a,
            r, g, b, a,   r, g, b, a,
            r, g, b, a,   r, g, b, a,
            r, g, b, a,   r, g, b, a
        };
        vertices.insert(vertices.end(), v.begin(), v.end());
        colors.insert(colors.end(), c.begin(), c.end());
    };
    std::function<void(const gfxm::vec3&, float, const gfxm::vec4&)> add_cross_fn = 
    [&vertices, &colors](const gfxm::vec3& pos, float sz, const gfxm::vec4& col){
        float sz_half = sz * 0.5f;
        std::vector<gfxm::vec3> v = {
            pos - gfxm::vec3(sz_half,0,0), pos + gfxm::vec3(sz_half,0,0),
            pos - gfxm::vec3(0,sz_half,0), pos + gfxm::vec3(0,sz_half,0)
        };
        uint8_t r = 255 * col.x;
        uint8_t g = 255 * col.y;
        uint8_t b = 255 * col.z;
        uint8_t a = 255 * col.w;
        std::vector<unsigned char> c = {
            r, g, b, a,   r, g, b, a,
            r, g, b, a,   r, g, b, a
        };
        vertices.insert(vertices.end(), v.begin(), v.end());
        colors.insert(colors.end(), c.begin(), c.end());
    };
    std::function<void(const gfxm::vec3&, float, const gfxm::vec4&)> add_diamond_fn = 
    [&vertices, &colors](const gfxm::vec3& pos, float sz, const gfxm::vec4& col){
        float sz_half = sz * 0.5f;
        gfxm::vec3 n = gfxm::vec3(pos.x, pos.y + sz_half, .0f);
        gfxm::vec3 s = gfxm::vec3(pos.x, pos.y - sz_half, .0f);
        gfxm::vec3 w = gfxm::vec3(pos.x - sz_half, pos.y, .0f);
        gfxm::vec3 e = gfxm::vec3(pos.x + sz_half, pos.y, .0f);
        std::vector<gfxm::vec3> v = {
            n, e,
            e, s,
            s, w,
            w, n
        };
        uint8_t r = 255 * col.x;
        uint8_t g = 255 * col.y;
        uint8_t b = 255 * col.z;
        uint8_t a = 255 * col.w;
        std::vector<unsigned char> c = {
            r, g, b, a,   r, g, b, a,
            r, g, b, a,   r, g, b, a,
            r, g, b, a,   r, g, b, a,
            r, g, b, a,   r, g, b, a
        };
        vertices.insert(vertices.end(), v.begin(), v.end());
        colors.insert(colors.end(), c.begin(), c.end());
    };
    
    if(gizmo_rect.cp_flags & GIZMO_RECT_NW) add_square_fn(gizmo_rect.nw, 8, gizmo_rect.cp == GIZMO_RECT_NW ? col_hover : col);
    if(gizmo_rect.cp_flags & GIZMO_RECT_NE) add_square_fn(gizmo_rect.ne, 8, gizmo_rect.cp == GIZMO_RECT_NE ? col_hover : col);
    if(gizmo_rect.cp_flags & GIZMO_RECT_SW) add_square_fn(gizmo_rect.sw, 8, gizmo_rect.cp == GIZMO_RECT_SW ? col_hover : col);
    if(gizmo_rect.cp_flags & GIZMO_RECT_SE) add_square_fn(gizmo_rect.se, 8, gizmo_rect.cp == GIZMO_RECT_SE ? col_hover : col);

    if(gizmo_rect.cp_flags & GIZMO_RECT_NORTH) add_square_fn(gfxm::lerp(gizmo_rect.nw, gizmo_rect.ne, 0.5f), 8, gizmo_rect.cp == GIZMO_RECT_NORTH ? col_hover : col);
    if(gizmo_rect.cp_flags & GIZMO_RECT_EAST) add_square_fn(gfxm::lerp(gizmo_rect.ne, gizmo_rect.se, 0.5f), 8, gizmo_rect.cp == GIZMO_RECT_EAST ? col_hover : col);
    if(gizmo_rect.cp_flags & GIZMO_RECT_SOUTH) add_square_fn(gfxm::lerp(gizmo_rect.se, gizmo_rect.sw, 0.5f), 8, gizmo_rect.cp == GIZMO_RECT_SOUTH ? col_hover : col);
    if(gizmo_rect.cp_flags & GIZMO_RECT_WEST) add_square_fn(gfxm::lerp(gizmo_rect.sw, gizmo_rect.nw, 0.5f), 8, gizmo_rect.cp == GIZMO_RECT_WEST ? col_hover : col);

    gfxm::vec3 origin 
        = gfxm::vec3(
            gfxm::lerp(gizmo_rect.nw.x, gizmo_rect.ne.x, gizmo_rect.origin.x), 
            gfxm::lerp(gizmo_rect.nw.y, gizmo_rect.sw.y, gizmo_rect.origin.y), 
            0
        );
    if(gizmo_rect.cp_flags & GIZMO_RECT_ORIGIN) add_cross_fn(origin, 16, gizmo_rect.cp == GIZMO_RECT_ORIGIN ? col_hover : col);
    if(gizmo_rect.cp_flags & GIZMO_RECT_ORIGIN) add_diamond_fn(origin, 12, gizmo_rect.cp == GIZMO_RECT_ORIGIN ? col_hover : col);

    gfxm::vec3 rotator_pos = gfxm::lerp(gizmo_rect.nw, gizmo_rect.ne, 0.5f) + gfxm::vec3(.0f, 20.0f, .0f);
    if(gizmo_rect.cp_flags & GIZMO_RECT_ROTATE) add_diamond_fn(rotator_pos, 12, gizmo_rect.cp == GIZMO_RECT_ROTATE ? col_hover : col);

    mesh.upload(VERTEX_FMT::ENUM_LINE::Position, vertices.data(), vertices.size() * sizeof(vertices[0]));
    mesh.upload(VERTEX_FMT::ENUM_LINE::ColorRGBA, colors.data(), colors.size() * sizeof(colors[0]));

    static gl::ShaderProgram* sh = shaderLoader().loadShaderProgram(
        "shaders/line.glsl", false, VERTEX_FMT::LINE::getVertexDesc()
    );
    sh->use();
    gfxm::mat4 model = gizmo_rect.transform;
    gfxm::mat4 view(1.0f);
    glUniformMatrix4fv(sh->getUniform("mat_proj"), 1, GL_FALSE, (float*)&proj);
    glUniformMatrix4fv(sh->getUniform("mat_model"), 1, GL_FALSE, (float*)&model);
    glUniformMatrix4fv(sh->getUniform("mat_view"), 1, GL_FALSE, (float*)&view);
    glUniform4fv(sh->getUniform("color"), 1, (float*)&color);

    mesh.bind();
    mesh.draw(GL_LINES);
}


void drawText(Font* fnt, const std::string& str, float screenW, float screenH, float x, float y) {
    const int FACE_HEIGHT = 26;
    std::vector<gfxm::vec3> vertices;
    std::vector<gfxm::vec2> uv;
    std::vector<float>      uv_lookup_indices;
    float horiAdvance = .0f;
    float lineOffset = fnt->getLineHeight(FACE_HEIGHT);
    for(int i = 0; i < str.size(); ++i) {
        if(str[i] == '\n') {
            lineOffset += fnt->getLineHeight(FACE_HEIGHT);
            horiAdvance = .0f;
            continue;
        }
        const auto& g = fnt->getGlyph(str[i], FACE_HEIGHT);
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
    glUniform1i(sh->getUniform("lookupTextureWidth"), fnt->getLookupTextureWidth(FACE_HEIGHT));
    
    GLuint texture = fnt->getAtlasTexture(FACE_HEIGHT);
    GLuint lookupTexture = fnt->getGlyphLookupTexture(FACE_HEIGHT);
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