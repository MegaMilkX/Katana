#include "resource_desc_library.hpp"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "../lib/stb_image_resize.h"

#include "resource/texture2d.h"
#include "scene/game_scene.hpp"
#include "resource/model_source.hpp"
#include "resource/audio_clip.hpp"
#include "resource/action_graph.hpp"
#include "resource/blend_tree.hpp"
#include "resource/material.hpp"

void ResourceDescLibrary::init() {
    add<GameScene>(
        "so", 
        FLAG_VIEWABLE | FLAG_WRITABLE
    )
    .add<Texture2D>(
        {"png", "jpg", "jpeg", "jfif", "tga"}, 
        FLAG_VIEWABLE
    )
    .add<AudioClip>(
        "ogg", 
        FLAG_VIEWABLE
    )
    .add<ModelSource>(
        {"fbx", "obj", "dae"}, 
        FLAG_VIEWABLE
    )
    .add<ActionGraph>(
        "action_graph", 
        FLAG_VIEWABLE | FLAG_WRITABLE
    )
    .add<BlendTree>(
        "blend_tree", 
        FLAG_VIEWABLE | FLAG_WRITABLE
    )
    .add<Material>(
        "mat", 
        FLAG_VIEWABLE | FLAG_WRITABLE
    );
}

std::string ResourceDescLibrary::getExtension(const std::string& res_name) {
    size_t dot_pos = res_name.find_last_of(".");
    if(dot_pos == std::string::npos) {
        LOG_WARN("ResourceDescLibrary: path has no extension - '" << res_name << "'");
        return 0;
    }
    if(dot_pos == res_name.size() - 1) {
        LOG_WARN("ResourceDescLibrary: path has no extension - '" << res_name << "'");
        return 0;
    }
    std::string ext = res_name.substr(dot_pos + 1);
    return ext;
}
rttr::type ResourceDescLibrary::findType(const std::string& res_path_or_ext) {
    std::string ext;
    if(res_path_or_ext.find_last_of(".") == std::string::npos) {
        ext = res_path_or_ext;
    } else {
        ext = getExtension(res_path_or_ext);
    }

    auto& it = ext_to_type.find(ext);
    if(it == ext_to_type.end()) {
        return rttr::type::get<void>();
    }
    return it->second;
}

std::shared_ptr<Texture2D> ResourceDescLibrary::createPreview(const std::string& res_path) {
    std::string ext = getExtension(res_path);
    if(ext.empty()) {
        return std::shared_ptr<Texture2D>();
    }
    rttr::type t = findType(ext);
    auto it = type_to_preview_creator.find(t);
    if(it == type_to_preview_creator.end()) {
        return std::shared_ptr<Texture2D>();
    }
    if (!it->second) {
      return std::shared_ptr<Texture2D>();
    }
    return it->second(res_path);
}

int ResourceDescLibrary::getFlags(rttr::type t) {
    auto it = type_flags.find(t);
    if(it == type_flags.end()) {
        return FLAG_NONE;
    }
    return it->second;
}
