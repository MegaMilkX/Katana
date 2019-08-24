#include "resource_desc_library.hpp"

#include "../../editor/editor_doc_scene.hpp"
#include "../../editor/editor_doc_texture2d.hpp"
#include "../../editor/doc_material.hpp"
#include "../../editor/editor_doc_audio_clip.hpp"
#include "../../editor/editor_doc_model_source.hpp"
#include "../../editor/doc_action_graph.hpp"

void ResourceDescLibrary::init() {
    add<GameScene>(
        "so", 
        FLAG_VIEWABLE | FLAG_WRITABLE, 
        [](std::shared_ptr<ResourceNode>& node)->EditorDocument*{
            return new EditorDocScene(node);
        }
    )
    .add<Texture2D>(
        {"png", "jpg", "jpeg", "jfif", "tga"}, 
        FLAG_VIEWABLE, 
        [](std::shared_ptr<ResourceNode>& node)->EditorDocument*{
            return new EditorDocTexture2d(node);
        }
    )
    .add<AudioClip>(
        "ogg", 
        FLAG_VIEWABLE, 
        [](std::shared_ptr<ResourceNode>& node)->EditorDocument*{
            return new EditorDocAudioClip(node);
        }
    )
    .add<ModelSource>(
        {"fbx", "obj", "dae"}, 
        FLAG_VIEWABLE, 
        [](std::shared_ptr<ResourceNode>& node)->EditorDocument*{
            return new EditorDocModelSource(node);
        }
    )
    .add<ActionGraph>(
        "action_graph", 
        FLAG_VIEWABLE | FLAG_WRITABLE, 
        [](std::shared_ptr<ResourceNode>& node)->EditorDocument*{
            return new DocActionGraph(node);
        }
    )
    .add<Material>(
        "mat", 
        FLAG_VIEWABLE | FLAG_WRITABLE, 
        [](std::shared_ptr<ResourceNode>& node)->EditorDocument*{
            return new DocMaterial(node);
        }
    );
}

rttr::type ResourceDescLibrary::findType(const std::string& ext) {
    auto& it = ext_to_type.find(ext);
    if(it == ext_to_type.end()) {
        return rttr::type::get<void>();
    }
    return it->second;
}

int ResourceDescLibrary::getFlags(rttr::type t) {
    auto it = type_flags.find(t);
    if(it == type_flags.end()) {
        return FLAG_NONE;
    }
    return it->second;
}

EditorDocument* ResourceDescLibrary::createEditorDocument(const std::string& res_path) {
    size_t dot_pos = res_path.find_last_of(".");
    if(dot_pos == std::string::npos) {
        LOG_WARN("createEditorDocument: path has no extension - '" << res_path << "'");
        return 0;
    }
    if(dot_pos == res_path.size() - 1) {
        LOG_WARN("createEditorDocument: path has no extension - '" << res_path << "'");
        return 0;
    }
    std::string ext = res_path.substr(dot_pos + 1);
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