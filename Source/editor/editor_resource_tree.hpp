#ifndef EDITOR_RESOURCE_TREE_HPP
#define EDITOR_RESOURCE_TREE_HPP

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <string>

const char* const IMGUI_DND_RESOURCE = "DND_RESOURCE";

class ResourceNode;
class Editor;
class EditorResourceTree {
    const ResourceNode* selected_node = 0;
    ResourceNode*       renaming_node = 0;
    bool needs_autoscroll = false;

    HANDLE dwChangeHandle;

    void initDirWatch(const std::string& dir);
    void checkDirChanges();

    void imguiContextMenu(const ResourceNode* node);

public:
    EditorResourceTree();
    ~EditorResourceTree();

    void setSelected(const ResourceNode*, bool auto_scroll = true);

    void update(Editor* editor);
};

#endif
