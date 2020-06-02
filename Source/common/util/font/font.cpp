#include "font.hpp"

#include <fstream>
#include <vector>
#include <assert.h>

#include "../log.hpp"

#include "../../lib/stb_image_write.h"
#include "../image/image.hpp"


void Font::loadFromMemory(void* data, size_t sz) {
    const int FACE_SIZE = 24;
    RectPack pack;
    FT_Error err = FT_New_Memory_Face(ftLib, (FT_Byte*)data, sz, 0, &face);
    if(err) {
        LOG_WARN("FT_New_Memory_Face failed " << err);
        assert(false);
        return;
    }
    err = FT_Select_Charmap(face, ft_encoding_unicode);
    assert(!err);
    err = FT_Set_Char_Size(face, 0, FACE_SIZE * 64.0f, 72, 72);
    assert(!err);
    /*
    std::vector<RectPack::Rect> rects;
    std::vector<std::vector<char>> bmps;
    const int GLYPH_COUNT = 64;
    rects.resize(GLYPH_COUNT);
    bmps.resize(GLYPH_COUNT);
    for (int i = 0; i < GLYPH_COUNT; ++i) {
        FT_UInt glyph_idx = FT_Get_Char_Index(face, 65 + i);
        assert(!FT_Load_Glyph(face, glyph_idx, FT_LOAD_DEFAULT | FT_LOAD_TARGET_NORMAL));
        if(face->glyph->format != FT_GLYPH_FORMAT_BITMAP) {
            assert(!FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL));
        }
        assert(face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY);

        size_t w = face->glyph->bitmap.width;
        size_t h = face->glyph->bitmap.rows;
        size_t bmp_byte_len = w * h;
        rects[i].w = w;
        rects[i].h = h;
        std::vector<char> bmpbuf(bmp_byte_len);
        memcpy(bmpbuf.data(), face->glyph->bitmap.buffer, bmp_byte_len);
        bmps[i] = bmpbuf;
    }
    RectPack::Rect r = pack.pack(rects.data(), rects.size(), RectPack::MAXSIDE, RectPack::POWER_OF_TWO);
    std::vector<char> buf(r.w * r.h);

    for (int i = 0; i < GLYPH_COUNT; ++i) {
        FT_UInt glyph_idx = FT_Get_Char_Index(face, 65 + i);
        assert(!FT_Load_Glyph(face, glyph_idx, FT_LOAD_DEFAULT | FT_LOAD_TARGET_NORMAL));
        if(face->glyph->format != FT_GLYPH_FORMAT_BITMAP) {
            assert(!FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL));
        }
        assert(face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY);

        size_t w = face->glyph->bitmap.width;
        size_t h = face->glyph->bitmap.rows;
        size_t bmp_byte_len = w * h;

        image_blit(
            buf.data(), r.w, r.h, 1,
            face->glyph->bitmap.buffer, w, h, 1,  rects[i].x, rects[i].y
        );
    }
    stbi_write_png(MKSTR("glyph.png").c_str(), r.w, r.h, 1, buf.data(), r.w);*/
}

void Font::rebuildAtlas () {
    RectPack pack;
    RectPack::Rect r = pack.pack(
        cached_rects.data(), cached_rects.size(), 
        RectPack::MAXSIDE, RectPack::POWER_OF_TWO
    );
    std::vector<char> buf(r.w * r.h);
    for (int i = 0; i < cached_characters.size(); ++i) {
        FT_UInt glyph_idx = FT_Get_Char_Index(face, cached_characters[i]);
        assert(!FT_Load_Glyph(face, glyph_idx, FT_LOAD_DEFAULT | FT_LOAD_TARGET_NORMAL));
        if(face->glyph->format != FT_GLYPH_FORMAT_BITMAP) {
            assert(!FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL));
        }
        assert(face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY);

        size_t w = face->glyph->bitmap.width;
        size_t h = face->glyph->bitmap.rows;
        size_t bmp_byte_len = w * h;
        image_blit(
            buf.data(), r.w, r.h, 1,
            face->glyph->bitmap.buffer, w, h, 1,  cached_rects[i].x, cached_rects[i].y
        );
    }
    atlasTexture->Data((unsigned char*)buf.data(), r.w, r.h, 1);
    //stbi_write_png(MKSTR("glyph.png").c_str(), r.w, r.h, 1, buf.data(), r.w);

    // Build uv lookup texture
    std::vector<gfxm::vec2> lookup;
    for(int i = 0; i < cached_rects.size(); ++i) {
        lookup.push_back(gfxm::vec2(
            cached_rects[i].x / r.w,
            (cached_rects[i].y + cached_rects[i].h) / r.h
        ));
        lookup.push_back(gfxm::vec2(
            (cached_rects[i].x + cached_rects[i].w) / r.w,
            (cached_rects[i].y + cached_rects[i].h) / r.h
        ));
        lookup.push_back(gfxm::vec2(
            (cached_rects[i].x + cached_rects[i].w) / r.w,
            cached_rects[i].y / r.h
        ));
        lookup.push_back(gfxm::vec2(
            cached_rects[i].x / r.w,
            cached_rects[i].y / r.h
        ));
    }
    lookupTexture->Data((float*)lookup.data(), lookup.size(), 1, 2);
    lookupTexture->Filter(GL_NEAREST);

    atlas_dirty = false;
}

Font::Glyph& Font::cacheGlyph  (uint32_t ch) {
    Glyph& g = glyphs[ch];

    FT_UInt glyph_idx = FT_Get_Char_Index(face, ch);
    FT_Error err = FT_Load_Glyph(face, glyph_idx, FT_LOAD_DEFAULT | FT_LOAD_TARGET_NORMAL);
    assert(!err);
    if(face->glyph->format != FT_GLYPH_FORMAT_BITMAP) {
        err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
        assert(!err);
    }
    assert(face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY);

    size_t w = face->glyph->bitmap.width;
    size_t h = face->glyph->bitmap.rows;
    size_t bmp_byte_len = w * h;

    g.width = w;
    g.height = h;
    g.horiAdvance = face->glyph->advance.x;
    g.bearingX = face->glyph->bitmap_left;
    g.bearingY = face->glyph->bitmap_top;
    g.cache_id = cached_characters.size();

    cached_characters.push_back(ch);
    RectPack::Rect rect;
    rect.w = w;
    rect.h = h;
    cached_rects.push_back(rect);

    atlas_dirty = true;
    return g;
}


Font::Font() {
    assert(!FT_Init_FreeType(&ftLib));
    atlasTexture.reset(new Texture2D());
    lookupTexture.reset(new Texture2D());
}
Font::~Font() {
    if(face) {
        FT_Done_Face(face);
    }
    FT_Done_FreeType(ftLib);
}

const Font::Glyph& Font::getGlyph(uint32_t ch) {
    auto it = glyphs.find(ch);
    if(it == glyphs.end()) {
        return cacheGlyph(ch);
    } else {
        return it->second;
    }
}

GLuint Font::getAtlasTexture() {
    if(atlas_dirty) {
        rebuildAtlas();
    }
    return atlasTexture->GetGlName();
}
GLuint Font::getGlyphLookupTexture() {
    if(atlas_dirty) {
        rebuildAtlas();
    }
    return lookupTexture->GetGlName();
}
int Font::getLookupTextureWidth() {
    return lookupTexture->Width();
}
float Font::getLineHeight() {
    return face->size->metrics.height / 64.0f;
}

bool Font::load(const char* fname) {
    std::ifstream file(fname, std::ios::binary | std::ios::ate);
    if(!file.is_open()) {
        return false;
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    file_buffer = std::vector<char>((unsigned int)size);
    if(!file.read(file_buffer.data(), size)) {
        return false;
    }
    loadFromMemory(file_buffer.data(), file_buffer.size());
    return true;
}

void fontFoo(const char* filename) {
    const char CH = 'A';
    const int FACE_SIZE = 24;

    int atlas_width = 0;
    int atlas_height = 0;
    int next_glyph_x = 0;
    int next_glyph_y = 0;

    RectPack pack;

    FT_Library ftLib;
    FT_Face face;

    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if(!file.is_open()) {
        return;
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> filebuffer = std::vector<char>((unsigned int)size);
    if(!file.read(filebuffer.data(), size)) {
        return;
    }

    FT_Error err = FT_Init_FreeType(&ftLib);
    assert(!err);
    err = FT_New_Memory_Face(ftLib, (FT_Byte*)filebuffer.data(), size, 0, &face);
    if(err) {
        LOG_WARN("FT_New_Memory_Face failed " << err);
        assert(false);
        return;
    }

    assert(!FT_Select_Charmap(face, ft_encoding_unicode));
    err = FT_Set_Char_Size(face, 0, FACE_SIZE * 64.0f, 100, 100);
    assert(!err);

    std::vector<RectPack::Rect> rects;
    std::vector<std::vector<char>> bmps;
    const int GLYPH_COUNT = 64;
    rects.resize(GLYPH_COUNT);
    bmps.resize(GLYPH_COUNT);
    for (int i = 0; i < GLYPH_COUNT; ++i) {
        FT_UInt glyph_idx = FT_Get_Char_Index(face, 65 + i);
        assert(!FT_Load_Glyph(face, glyph_idx, FT_LOAD_DEFAULT | FT_LOAD_TARGET_NORMAL));
        if(face->glyph->format != FT_GLYPH_FORMAT_BITMAP) {
            assert(!FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL));
        }
        assert(face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY);

        size_t w = face->glyph->bitmap.width;
        size_t h = face->glyph->bitmap.rows;
        size_t bmp_byte_len = w * h;
        rects[i].w = w;
        rects[i].h = h;
        std::vector<char> bmpbuf(bmp_byte_len);
        memcpy(bmpbuf.data(), face->glyph->bitmap.buffer, bmp_byte_len);
        bmps[i] = bmpbuf;
    }
    RectPack::Rect r = pack.pack(rects.data(), rects.size(), RectPack::MAXSIDE, RectPack::POWER_OF_TWO);
    std::vector<char> buf(r.w * r.h);

    for (int i = 0; i < GLYPH_COUNT; ++i) {
        FT_UInt glyph_idx = FT_Get_Char_Index(face, 65 + i);
        assert(!FT_Load_Glyph(face, glyph_idx, FT_LOAD_DEFAULT | FT_LOAD_TARGET_NORMAL));
        if(face->glyph->format != FT_GLYPH_FORMAT_BITMAP) {
            assert(!FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL));
        }
        assert(face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY);

        size_t w = face->glyph->bitmap.width;
        size_t h = face->glyph->bitmap.rows;
        size_t bmp_byte_len = w * h;

        image_blit(
            buf.data(), r.w, r.h, 1,
            face->glyph->bitmap.buffer, w, h, 1,  rects[i].x, rects[i].y
        );
    }
    stbi_write_png(MKSTR("glyph.png").c_str(), r.w, r.h, 1, buf.data(), r.w);

    

    FT_Done_FreeType(ftLib);
}