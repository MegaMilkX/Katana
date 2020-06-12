#ifndef FONT_HPP
#define FONT_HPP

#include "resource.h"

#include <stdint.h>
#include <unordered_map>
#include "texture2d.h"
#include <ft2build.h>
#include FT_FREETYPE_H

#include "../util/rect_pack.hpp"


class Font : public Resource {
public:
    struct Glyph {
        float width;
        float height;
        float horiAdvance;
        float bearingX;
        float bearingY;

        uint64_t cache_id;
    };
    struct AtlasData {
        std::vector<uint32_t> cached_characters;
        std::vector<RectPack::Rect> cached_rects;
        std::shared_ptr<Texture2D> atlasTexture;
        std::shared_ptr<Texture2D> lookupTexture;
        std::unordered_map<uint32_t, Glyph> glyphs;
        bool dirty = false;
    };

private:
    FT_Library ftLib;
    FT_Face face;
    std::vector<char> file_buffer;
    std::unordered_map<uint16_t, std::unique_ptr<AtlasData>> atlas_per_size;

    void loadFromMemory(void* data, size_t sz);
    void rebuildAtlas (uint16_t font_height);
    Glyph& cacheGlyph  (uint32_t ch, uint16_t font_height);
    AtlasData* getAtlasData(uint16_t font_height);

public:
    Font();
    ~Font();

    const Glyph& getGlyph(uint32_t ch, uint16_t font_height);

    GLuint getAtlasTexture(uint16_t font_height);
    GLuint getGlyphLookupTexture(uint16_t font_height);
    int    getLookupTextureWidth(uint16_t font_height);
    float  getLineHeight(uint16_t font_height);

    bool load(const char* fname);

    bool deserialize(in_stream& in, size_t sz) override;
};
STATIC_RUN(Font) {
    rttr::registration::class_<Font>("Font")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
