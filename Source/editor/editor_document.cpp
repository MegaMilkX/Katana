#include "editor_document.hpp"

#include "editor.hpp"

#include "../common/resource/resource_tree.hpp"

#include "../common/util/filesystem.hpp"
#include "../common/platform/platform.hpp"

#include "../common/resource/ext_to_icon.hpp"

#include "../common/util/filesystem.hpp"
#include "../common/util/log.hpp"

typedef std::map<std::string, rttr::type> ext_to_type_map_t;
ext_to_type_map_t& getExtToTypeMap() {
    static ext_to_type_map_t map;
    return map;
}

void regEditorDocument(rttr::type doc_type, const std::vector<std::string>& file_extensions) {
    for(auto& ext : file_extensions) {
        getExtToTypeMap().insert(std::make_pair(ext, doc_type));
    }
}

EditorDocument* createEditorDocument(rttr::type resource_type) {
    EditorDocument* doc = rttrCreate<EditorDocument>(resource_type);
    if(!doc) {
        LOG_WARN("createEditorDocument (from type): " << resource_type.get_name().to_string() << " failed");
        return 0;
    }

    return doc;
}

EditorDocument* createEditorDocument(const std::string& resource_path) {
    std::string ext = get_extension(resource_path);
    if(ext.empty()) {
        LOG_WARN("createEditorDocument: '" << resource_path << "' path has no extension");
        return 0;
    }

    auto it = getExtToTypeMap().find(ext);
    if(it == getExtToTypeMap().end()) {
        LOG_WARN("createEditorDocument: '" << resource_path << "' no registered document available for this type");
        return 0;
    }

    EditorDocument* doc = rttrCreate<EditorDocument>(it->second);
    if(!doc) {
        LOG_WARN("createEditorDocument: '" << resource_path << "' failed to open document");
        return 0;
    }

    doc->setResource(resource_path);

    return doc;
}


EditorDocument::EditorDocument()
{
    markUnsaved();
    setTitle(MKSTR("Untitled" << "###" << this));
}

void EditorDocument::setResource(const std::string& path) {
    setResourceNode(gResourceTree.find_shared(path));
}

ResourceNode* EditorDocument::getNode() {
    return _node.lock().get();
}

bool EditorDocument::saveResource(std::shared_ptr<Resource>& r, const std::string& path) {
    onPreSave();
    
    if(r->write_to_file(path)) {
        gResourceTree.scanFilesystem(get_module_dir() + "/" + platformGetConfig().data_dir);
        return true;
    }
    return false;
}

