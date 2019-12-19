#ifndef EDITOR_DOCUMENT_HPP
#define EDITOR_DOCUMENT_HPP

#include <string>
#include <memory>

#include "editor_window.hpp"

#include "../common/util/log.hpp"

#include "../common/util/filesystem.hpp"

#include "../common/platform/platform.hpp"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <shlwapi.h>

#include "dialog_save.hpp"

#include "../common/lib/rttr_wrap.hpp"

#include "../common/resource/resource_tree.hpp"

class Resource;
class ResourceNode;
class EditorDocument : public EditorWindow {
protected:
    std::weak_ptr<ResourceNode> _node;

    virtual void setResourceNode(std::shared_ptr<ResourceNode>& node) = 0;

public:
    EditorDocument();
    virtual ~EditorDocument() {}

    void setResource(const std::string& path);

    ResourceNode* getNode();

    bool saveResource(std::shared_ptr<Resource>& r, const std::string& path);

    virtual void onPreSave() {

    }

};

inline std::string sanitizeString(const std::string& str) {
    std::string name = str;
    for(size_t i = 0; i < name.size(); ++i) {
        name[i] = (std::tolower(name[i]));
        if(name[i] == '\\') {
            name[i] = '/';
        }
    }
    return name;
}

template<typename T>
class EditorDocumentTyped : public EditorDocument {
protected:
    std::shared_ptr<T> _resource;

    void setResourceNode(std::shared_ptr<ResourceNode>& node) override {
        if(!node) return;

        setTitle(MKSTR(node->getFullName() << "###" << this));
        setName(node->getFullName());
        _node = node;
        _resource = node->getResource<T>();
        
        std::string ext;
        if(getName().find_last_of(".") != getName().npos) {
            ext = getName().substr(getName().find_last_of("."));
            setIconCode(getExtIconCode(ext.c_str()));
        }

        onResourceSet();
    }

public:
    EditorDocumentTyped() {
        _resource.reset(new T());
    }
    ~EditorDocumentTyped() {
        _resource.reset();
        gResourceTree.getRoot()->tryReleaseRecursive();
    }

    virtual void save() {
        LOG("Saving document...");

        if(std::string(_resource->getWriteExtension()).empty()) {
            LOG("Resource is not writeable");
            return;
        }

        std::string save_name;
        if(_resource->Name().empty() || _node.expired()) {
            save_name = dialogSave(_resource->getWriteExtension());
        } else {
            save_name = _resource->Name();
            save_name = get_module_dir() + "/" + platformGetConfig().data_dir + "/" + save_name;
        }

        if(!saveResource(std::dynamic_pointer_cast<Resource>(_resource), save_name)) {
            LOG("Zero bytes written");
            return;
        }

        if(_resource->Name().empty() || _node.expired()) {
            char buf[MAX_PATH];
            PathRelativePathToA(buf, (get_module_dir() + "/" + platformGetConfig().data_dir).c_str(), FILE_ATTRIBUTE_DIRECTORY, save_name.c_str(), FILE_ATTRIBUTE_NORMAL);
            std::string rel_path = sanitizeString(buf);
            auto data_dir = platformGetConfig().data_dir;
            if(!data_dir.empty()) {
                if(strncmp(data_dir.c_str(), rel_path.c_str(), data_dir.size()) == 0) {
                    rel_path = rel_path.substr(data_dir.size() + 1);
                }
            }
            _resource->Name(rel_path);
            setName(rel_path);
            _node = gResourceTree.find_shared(rel_path);
            if(!_node.expired()) {
                _node.lock()->overrideResource(_resource);
                setTitle(MKSTR(rel_path << "###" << this));
            } else {
                LOG_WARN("Save failed");
            }
        }

        LOG("Done");
    }

    virtual void onResourceSet() {
        
    }
};

void regEditorDocument(rttr::type doc_type, const std::vector<std::string>& file_extensions);

template<typename DOC_T>
void regEditorDocument(const std::vector<std::string>& file_extensions) {
    rttr::registration::class_<DOC_T>(rttr::type::get<DOC_T>().get_name().to_string())
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
    regEditorDocument(rttr::type::get<DOC_T>(), file_extensions);
}
template<typename DOC_T>
void regEditorDocument(const char* file_extension) {
    regEditorDocument<DOC_T>({ std::string(file_extension) });
}

EditorDocument* createEditorDocument(const std::string& resource_path);


#endif
