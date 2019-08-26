#ifndef PREVIEW_LIBRARY_HPP
#define PREVIEW_LIBRARY_HPP

#include <memory>
#include <string>

#include "../common/resource/texture2d.h"

#include "../common/lib/sqlite/sqlite3.h"

#include "../common/util/singleton.hpp"

class PreviewLibrary : public Singleton<PreviewLibrary> {
    sqlite3* _db;
    std::shared_ptr<Texture2D> no_preview_tex;
    std::map<std::string, std::shared_ptr<Texture2D>> loaded_thumbs;

public:
    PreviewLibrary();
    ~PreviewLibrary();
    std::shared_ptr<Texture2D> getPreview(const std::string& res_path);
    std::shared_ptr<Texture2D> getPreviewPlaceholder();
};

#endif
