#ifndef FONT_HPP
#define FONT_HPP

#include <stdint.h>
#include <unordered_map>
#include "../../resource/texture2d.h"
#include <ft2build.h>
#include FT_FREETYPE_H

#include "../rect_pack.hpp"

void fontFoo(const char* filename);


class Font {
public:
    struct Glyph {
        float width;
        float height;
        float horiAdvance;
        float bearingX;
        float bearingY;

        uint64_t cache_id;
    };

private:
    FT_Library ftLib;
    FT_Face face;
    std::vector<char> file_buffer;
    std::vector<uint32_t> cached_characters;
    std::vector<RectPack::Rect> cached_rects;
    std::shared_ptr<Texture2D> atlasTexture;
    std::shared_ptr<Texture2D> lookupTexture;
    bool atlas_dirty = false;

    std::unordered_map<uint32_t, Glyph> glyphs;

    void loadFromMemory(void* data, size_t sz);
    void rebuildAtlas ();
    Glyph& cacheGlyph  (uint32_t ch);

public:
    Font();
    ~Font();

    const Glyph& getGlyph(uint32_t ch);

    GLuint getAtlasTexture();
    GLuint getGlyphLookupTexture();
    int    getLookupTextureWidth();
    float  getLineHeight();

    bool load(const char* fname);
};

#endif
