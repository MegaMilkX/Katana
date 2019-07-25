#include "editor_resource_tree.hpp"

#include "../common/lib/imgui_wrap.hpp"
#include "../common/lib/imgui/imgui_internal.h"
#include "../common/util/log.hpp"

#include "../common/resource/resource_tree.hpp"

#include "editor.hpp"
#include "editor_doc_scene.hpp"
#include "editor_doc_texture2d.hpp"

#include "../common/util/has_suffix.hpp"

#include "../common/platform/platform.hpp"

#define NO_MIN_MAX
#define WIN32_LEAN_AND_MEAN
#include <shellapi.h>
#include <Shlobj.h>

void EditorResourceTree::initDirWatch(const std::string& dir) {
    dwChangeHandle = FindFirstChangeNotificationA(
        dir.c_str(),
        TRUE,
        FILE_NOTIFY_CHANGE_CREATION |
        FILE_NOTIFY_CHANGE_DIR_NAME |
        FILE_NOTIFY_CHANGE_FILE_NAME
    );
    DWORD err = GetLastError();
    if(dwChangeHandle == INVALID_HANDLE_VALUE) {
        LOG_ERR("FindFirstChangeNotificationA failed: " << dwChangeHandle);
        LOG_ERR("GetLastError: " << err);
        return;
    }
}

void EditorResourceTree::checkDirChanges() {
    DWORD dwWaitStatus = WaitForSingleObject(dwChangeHandle, 0);
    switch(dwWaitStatus) {
    case WAIT_OBJECT_0:
        gResourceTree.scanFilesystem(get_module_dir() + "/" + platformGetConfig().data_dir);

        if(FindNextChangeNotification(dwChangeHandle) == FALSE) {
            LOG_ERR("FindNextChangeNotification failed");
            return;
        }
        break;
    }
}


EditorResourceTree::EditorResourceTree() {
    initDirWatch(get_module_dir() + "/" + platformGetConfig().data_dir);
}
EditorResourceTree::~EditorResourceTree() {

}

void EditorResourceTree::setSelected(const ResourceNode* n, bool auto_scroll) {
    if(n == selected_node) return;

    selected_node = n;
    if(auto_scroll) {
        needs_autoscroll = true;
    }
}

static void imguiDragDropSource(const std::string& name, void* payload, const std::string& label) {
    if(ImGui::BeginDragDropSource(0)) {
        ImGui::SetDragDropPayload(name.c_str(), &payload, sizeof(payload));
        ImGui::Text(label.c_str());
        ImGui::EndDragDropSource();
    }
}

static void imguiDragDropTarget(const std::string& name) {
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(name.c_str())) {
            //GameObject* tgt_dnd_so = *(GameObject**)payload->Data;
            //o->takeOwnership(tgt_dnd_so);
        }
        ImGui::EndDragDropTarget();
    }
}

static void imguiContextMenu(const ResourceNode* node) {
    if (ImGui::BeginPopupContextItem()) {
        if(ImGui::MenuItem("Open in default program")) {
            ShellExecuteA(NULL, "open", MKSTR(get_module_dir() + "/" + platformGetConfig().data_dir + "/" + node->getFullName()).c_str(), NULL, NULL, SW_SHOWDEFAULT);
        }
        if(ImGui::MenuItem("Show in Explorer")) {
            std::string path = MKSTR(get_module_dir() + "/" + platformGetConfig().data_dir + "/" + node->getFullName());
            for(auto& c : path) {
                if(c == '/') c = '\\';
            }
            char buf[MAX_PATH];
            GetFullPathNameA(path.c_str(), MAX_PATH, buf, 0);
            path = buf;
            LOG(path);

            CoInitialize(NULL);
            ITEMIDLIST *pidl = ILCreateFromPathA(path.c_str());
            if(pidl) {
                SHOpenFolderAndSelectItems(pidl,0,0,0);
                ILFree(pidl);
            } else {
                LOG_WARN("Failed to open explorer to show '" << path << "'");
            }
        }
        ImGui::Separator();
        if(ImGui::MenuItem("Cut")) {
            
        }
        if(ImGui::MenuItem("Copy")) {
            
        }
        ImGui::Separator();
        if(ImGui::MenuItem("Rename")) {
            
        }
        if(ImGui::MenuItem("Delete")) {
            
        }
        ImGui::EndPopup();
    }
}

void EditorResourceTree::update(Editor* editor) {
    checkDirChanges();

    if(ImGui::Begin("Resource Tree")) {
        ImGui::TextWrapped("Select a resource to see it's description and preview here");
        ImGui::BeginChildFrame(ImGui::GetID("ResourceTreeTree"), ImVec2(0,0));
        std::function<void(const ResourceNode*/*, const std::set<const ResourceNode*>& */)> imguiResourceTree;
        imguiResourceTree = [this, editor, &imguiResourceTree](const ResourceNode* node/*, const std::set<const ResourceNode*>& valid_nodes */) {
            /*
            if(valid_nodes.find(node) == valid_nodes.end()) {
                return;
            } */
            std::string node_label = node->getName();
            if(node->isLoaded()) {
                node_label += " [L]";
            }
            if(node->childCount()) {
                if(ImGui::TreeNodeEx(
                    (void*)node,
                    ImGuiTreeNodeFlags_OpenOnDoubleClick |
                    ImGuiTreeNodeFlags_OpenOnArrow,
                    node_label.c_str()
                )) {
                    for(auto& kv : node->getChildren()) {
                        imguiResourceTree(kv.second.get()/*, valid_nodes */);
                    }
                    ImGui::TreePop();
                }
            } else {
                bool selected = selected_node == node;
                if(selected && needs_autoscroll) {
                    needs_autoscroll = false;
                    ImGui::SetScrollY(ImGui::GetCursorScreenPos().y);
                }
                if(ImGui::Selectable(node_label.c_str(), &selected, ImGuiSelectableFlags_AllowDoubleClick)) {
                    if(ImGui::IsMouseDoubleClicked(0)) {
                        editor->tryOpenDocument(node);
                        setSelected(node, false);
                    }
                }
                imguiDragDropSource(IMGUI_DND_RESOURCE, (void*)node, node->getName());
                imguiDragDropTarget(IMGUI_DND_RESOURCE);
                imguiContextMenu(node);
            }
        };

        for(auto& kv : gResourceTree.getRoot()->getChildren()) {
            imguiResourceTree(kv.second.get());
        }
        ImGui::EndChildFrame();

        ImGui::End();
    }
}