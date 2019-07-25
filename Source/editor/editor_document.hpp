#ifndef EDITOR_DOCUMENT_HPP
#define EDITOR_DOCUMENT_HPP

#include <string>

#include "../common/lib/imgui_wrap.hpp"
#include "../common/lib/imgui/imgui_internal.h"
#include "../common/util/log.hpp"

class ResourceNode;
class Editor;
class EditorDocument {
    std::string _name;
    ResourceNode* _node;
public:
    EditorDocument(ResourceNode* node);
    virtual ~EditorDocument() {}

    const std::string& getName() const;

    bool isOpen() const { return is_open; }
    
    void update(Editor* ed);

    virtual void onGui(Editor* ed) = 0;
protected:
    bool is_open = true;
};

#endif
