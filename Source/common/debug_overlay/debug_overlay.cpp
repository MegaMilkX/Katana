#include "debug_overlay.hpp"

#include <string>
#include <vector>
#include "../gen/nimbusmono_bold.otf.h"
#include "../resource/font.hpp"
#include "../render/shader_loader.hpp"
#include "../render/vertex_format.hpp"

#include "../stats/frame_stats.hpp"

void drawText(Font* fnt, const std::string& str, float screenW, float screenH, float x, float y) {
    const int FACE_HEIGHT = 14;
    std::vector<gfxm::vec3> vertices;
    std::vector<gfxm::vec2> uv;
    std::vector<float>      uv_lookup_indices;
    std::vector<gfxm::vec4> colors;
    int horiAdvance = 0;
    int lineOffset = fnt->getLineHeight(FACE_HEIGHT);
    int advSpace  = fnt->getGlyph('\s', FACE_HEIGHT).horiAdvance;
    int advTab    = advSpace * 8;
    
    gfxm::vec4 color(1.0f, 1.0f, 1.0f, 1.0f);

    for(int i = 0; i < str.size(); ++i) {
        if(str[i] == '\n') {
            lineOffset += fnt->getLineHeight(FACE_HEIGHT);
            horiAdvance = 0;
            continue;
        } else if (str[i] == '\t') {
            int reminder = horiAdvance % (advTab / 64);
            int adv = advTab / 64 - reminder;
            horiAdvance += adv;
            continue;
        } else if (str[i] == '#') {
            int characters_left = str.size() - i - 1;
            if (characters_left >= 8) {
                std::string str_col(&str[i + 1], &str[i + 1] + 8);
                uint64_t l_color = strtoll(str_col.c_str(), 0, 16);
                color.r = ((l_color & 0xff000000) >> 24) / 255.0f;
                color.g = ((l_color & 0x00ff0000) >> 16) / 255.0f;
                color.b = ((l_color & 0x0000ff00) >> 8) / 255.0f;
                color.a = (l_color & 0x000000ff) / 255.0f;
                i += 8;
                continue;
            }
        }
        const auto& g = fnt->getGlyph(str[i], FACE_HEIGHT);
        int y_ofs = g.height - g.bearingY;
        int x_ofs = g.bearingX;
        vertices.push_back(gfxm::vec3(horiAdvance + x_ofs,           0 - y_ofs - lineOffset,        0));
        vertices.push_back(gfxm::vec3(horiAdvance + g.width + x_ofs, 0 - y_ofs - lineOffset,        0));
        vertices.push_back(gfxm::vec3(horiAdvance + x_ofs,           g.height - y_ofs - lineOffset, 0));
        vertices.push_back(gfxm::vec3(horiAdvance + g.width + x_ofs, 0 - y_ofs - lineOffset,        0));
        vertices.push_back(gfxm::vec3(horiAdvance + g.width + x_ofs, g.height - y_ofs - lineOffset, 0));
        vertices.push_back(gfxm::vec3(horiAdvance + x_ofs,           g.height - y_ofs - lineOffset, 0));
        horiAdvance += g.horiAdvance / 64;

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

        colors.push_back(color);
        colors.push_back(color);
        colors.push_back(color);
        colors.push_back(color);
        colors.push_back(color);
        colors.push_back(color);
    }

    int vertex_stride = sizeof(float) * 3;
    
    GLuint vao = 0;
    GLuint vbuf = 0;
    GLuint uvbuf = 0;
    GLuint uvlookupbuf = 0;
    GLuint colorbuf = 0;
    glGenBuffers(1, &vbuf);
    glGenBuffers(1, &uvbuf);
    glGenBuffers(1, &uvlookupbuf);
    glGenBuffers(1, &colorbuf);

    glGenVertexArrays(1, &vao);
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

    glBindBuffer(GL_ARRAY_BUFFER, colorbuf);
    glEnableVertexAttribArray(VFMT::ENUM_TEXT::ColorRGBA);
    glVertexAttribPointer(
      VFMT::ENUM_TEXT::ColorRGBA, 4, GL_FLOAT, GL_FALSE,
      sizeof(float) * 4, 0
    );
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(gfxm::vec4), colors.data(), GL_STREAM_DRAW);

    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, screenW, screenH);
    static gl::ShaderProgram* sh = shaderLoader().loadShaderProgram("shaders/text.glsl", false, VFMT::TEXT::getVertexDesc());
    sh->use();
    gfxm::mat4 proj = gfxm::ortho(0.0f, screenW, -screenH, 0.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(sh->getUniform("mat_proj"), 1, GL_FALSE, (float*)&proj);
    gfxm::mat4 model = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(x, y, 0));
    glUniformMatrix4fv(sh->getUniform("mat_model"), 1, GL_FALSE, (float*)&model);
    glUniform1i(sh->getUniform("lookupTextureWidth"), fnt->getLookupTextureWidth(FACE_HEIGHT));
    glUniform4f(sh->getUniform("color"), 1.0f, 1.0f, 1.0f, 1.0f);
    
    GLuint texture = fnt->getAtlasTexture(FACE_HEIGHT);
    GLuint lookupTexture = fnt->getGlyphLookupTexture(FACE_HEIGHT);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, lookupTexture);

    model = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(x, y - 2.0f, 0));
    glUniformMatrix4fv(sh->getUniform("mat_model"), 1, GL_FALSE, (float*)&model);
    glUniform4f(sh->getUniform("color"), .0f, .0f, .0f, 1.0f);
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());

    model = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(x, y, 0));
    glUniformMatrix4fv(sh->getUniform("mat_model"), 1, GL_FALSE, (float*)&model);
    glUniform4f(sh->getUniform("color"), 1.0f, 1.0f, 1.0f, 1.0f);
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());

    

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbuf);
    glDeleteBuffers(1, &uvbuf);
    glDeleteBuffers(1, &uvlookupbuf);
    glDeleteBuffers(1, &colorbuf);
}

static std::unique_ptr<Font> dbgOverlayFont;
void debugOverlayInit() {
    dbgOverlayFont.reset(new Font);
    dstream strm;
    strm.write(nimbusmono_bold_otf, sizeof(nimbusmono_bold_otf));
    strm.jump(0);
    dbgOverlayFont->deserialize(strm, strm.bytes_available());
}

#include "../input2/input2.hpp"
void debugOverlayDraw(unsigned screen_w, unsigned screen_h) {
    std::string text = MKSTR(
        "#ccccccff  FPS:\t\t#ffffffff" << round(gFrameStats.fps) <<
        "#ccccccff\n  Frame time:\t#00ff00ff" << gFrameStats.frame_time <<
        "#ffffffff"
    );
    text += "\n\n\tInput buffer:\n";
    InputCmd cmds[INPUT_CMD_BUFFER_LENGTH];
    inputGetBufferSnapshot(cmds, INPUT_CMD_BUFFER_LENGTH);
    for(int i = 0; i < INPUT_CMD_BUFFER_LENGTH; ++i) {
        auto& c = cmds[i];
        text += MKSTR("[X]\t" << (int)c.device_type << "\t" << c.key << "\t" << c.value << "\n").c_str();
    }

    std::string text2 = "Action Event Log:\n";
    InputActionEvent events[INPUT_ACTION_EVENT_BUFFER_LENGTH];
    inputGetActionEventBufferSnapshot(events, INPUT_ACTION_EVENT_BUFFER_LENGTH);
    for(int i = 0; i < INPUT_ACTION_EVENT_BUFFER_LENGTH; ++i) {
        auto& e = events[i];
        text2 += MKSTR(e.name << "\t" << inputActionEventTypeToString((InputActionEventType)e.type) << "\n");
    }

    std::string text3 = "Context Stack:\n";
    InputContext* context_stack[10] = { 0 };
    int context_count = inputGetContextStack(context_stack, 10);
    for(int i = context_count - 1; i >= 0; --i) {
        auto ctx = context_stack[i];
        text3 += MKSTR(ctx->getName() << "\n");
        for(auto& range : ctx->getRanges()) {
            text3 += MKSTR("\t" << range->getName() << "\t" << range->getVec3() << "\n");
        }
        for(auto& action : ctx->getActions()) {
            text3 += MKSTR("\t" << action->getName() << "\t[" << (action->isPressed() ? "X" : " ") << "]\n");
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    drawText(
        dbgOverlayFont.get(),
        text.c_str(), 
        screen_w, screen_h,
        0, 0
    );
    
    drawText(
        dbgOverlayFont.get(),
        text2.c_str(),
        screen_w, screen_h,
        500, 0
    );

    drawText(
        dbgOverlayFont.get(),
        text3.c_str(),
        screen_w, screen_h,
        700, 0
    );
}