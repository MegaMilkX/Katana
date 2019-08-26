#ifndef EDITOR_DOCUMENT_HPP
#define EDITOR_DOCUMENT_HPP

#include <string>
#include <memory>

#include "../common/lib/imgui_wrap.hpp"
#include "../common/lib/imgui/imgui_internal.h"
#include "../common/util/log.hpp"

#include "../common/util/filesystem.hpp"

#include "../common/platform/platform.hpp"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <shlwapi.h>

#include "dialog_save.hpp"

class Resource;
class ResourceNode;
class Editor;
class EditorDocument {
protected:
    std::string _name;
    std::string _window_name;
    std::weak_ptr<ResourceNode> _node;
    ImGuiWindowFlags imgui_win_flags = 0;
public:
    EditorDocument();
    virtual ~EditorDocument() {}

    virtual void onFocus() {}

    virtual void setResourceNode(std::shared_ptr<ResourceNode>& node) = 0;

    ResourceNode* getNode();

    const std::string& getName() const;
    const std::string& getWindowName() const;

    bool isOpen() const { return is_open; }
    bool isUnsaved() const { return is_unsaved; }
    void setOpen(bool b) { is_open = b; }

    void markUnsaved() { is_unsaved = true; }

    virtual void save() = 0;
    void saveAs();
    bool saveResource(std::shared_ptr<Resource>& r, const std::string& path);

    void undo();
    void redo();
    void backup();
    
    void update(Editor* ed, float dt);

    virtual void onGui(Editor* ed, float dt) = 0;
    virtual void onGuiToolbox(Editor* ed) {}
protected:
    bool is_open = true;
    bool is_unsaved = false;
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
            _name = rel_path;
            _node = gResourceTree.find_shared(rel_path);
            if(!_node.expired()) {
                _node.lock()->overrideResource(_resource);
                _window_name = MKSTR(rel_path << "###" << this);
            } else {
                LOG_WARN("Save failed");
            }
        }

        LOG("Done");
    }

    virtual void setResourceNode(std::shared_ptr<ResourceNode>& node) {
        if(!node) return;

        _window_name = MKSTR(node->getFullName() << "###" << this);
        _name = node->getFullName();
        _node = node;
        _resource = node->getResource<T>();
    }
};

#endif
