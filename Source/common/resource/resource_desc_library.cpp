#include "resource_desc_library.hpp"

#include "../../editor/editor_doc_scene.hpp"
#include "../../editor/editor_doc_texture2d.hpp"
#include "../../editor/doc_material.hpp"
#include "../../editor/editor_doc_audio_clip.hpp"
#include "../../editor/editor_doc_model_source.hpp"
#include "../../editor/doc_action_graph.hpp"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "../lib/stb_image_resize.h"

void ResourceDescLibrary::init() {
    add<GameScene>(
        "so", 
        FLAG_VIEWABLE | FLAG_WRITABLE, 
        [](std::shared_ptr<ResourceNode>& node)->EditorDocument*{
#ifdef KT_EDITOR
            return new EditorDocScene(node);
#else
            return 0;
#endif
        }
    )
    .add<Texture2D>(
        {"png", "jpg", "jpeg", "jfif", "tga"}, 
        FLAG_VIEWABLE, 
        [](std::shared_ptr<ResourceNode>& node)->EditorDocument*{
#ifdef KT_EDITOR
            return new EditorDocTexture2d(node);
#else
            return 0;
#endif
        },
        [](const std::string& res_name)->std::shared_ptr<Texture2D>{
            std::shared_ptr<Texture2D> tex = retrieve<Texture2D>(res_name);
            std::shared_ptr<Texture2D> out_tex(new Texture2D());
            std::vector<unsigned char> buf;
            buf.resize(256 * 256 * tex->getBpp());
            stbir_resize_uint8(
                tex->getData(), tex->Width(), tex->Height(), 0,
                buf.data(), 256, 256, 0, tex->getBpp()
            );
            out_tex->Data(buf.data(), 256, 256, tex->getBpp());
            return out_tex;
        }
    )
    .add<AudioClip>(
        "ogg", 
        FLAG_VIEWABLE, 
        [](std::shared_ptr<ResourceNode>& node)->EditorDocument*{
#ifdef KT_EDITOR
            return new EditorDocAudioClip(node);
#else
            return 0;
#endif
        }
    )
    .add<ModelSource>(
        {"fbx", "obj", "dae"}, 
        FLAG_VIEWABLE, 
        [](std::shared_ptr<ResourceNode>& node)->EditorDocument*{
#ifdef KT_EDITOR
            return new EditorDocModelSource(node);
#else
            return 0;
#endif
        }
    )
    .add<ActionGraph>(
        "action_graph", 
        FLAG_VIEWABLE | FLAG_WRITABLE, 
        [](std::shared_ptr<ResourceNode>& node)->EditorDocument*{
#ifdef KT_EDITOR
            return new DocActionGraph(node);
#else
            return 0;
#endif
        }
    )
    .add<Material>(
        "mat", 
        FLAG_VIEWABLE | FLAG_WRITABLE, 
        [](std::shared_ptr<ResourceNode>& node)->EditorDocument*{
#ifdef KT_EDITOR
            return new DocMaterial(node);
#else
            return 0;
#endif
        }
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

EditorDocument* ResourceDescLibrary::createEditorDocument(const std::string& res_path) {
    std::string ext = getExtension(res_path);
    if(ext.empty()) {
        return 0;
    }
    auto& ext_it = ext_to_type.find(ext);
    if(ext_it == ext_to_type.end()) {
        LOG_WARN("createEditorDocument: '" << ext << "' - extension is not registered");
        return 0;
    }
    auto node = gResourceTree.find_shared(res_path);
    if(!node) {
        LOG_WARN("createEditorDocument: can't find resource node - '" << res_path << "'");
        return 0;
    }
    return createEditorDocument(ext_it->second, node);
}
EditorDocument* ResourceDescLibrary::createEditorDocument(rttr::type t, std::shared_ptr<ResourceNode>& rnode) {
    auto& it = type_to_doc_creator.find(t);
    if(it == type_to_doc_creator.end()) {
        return 0;
    }
    return it->second(rnode);
}