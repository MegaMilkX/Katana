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

#include "../common/resource/ext_to_icon.hpp"

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
            //ktNode* tgt_dnd_so = *(ktNode**)payload->Data;
            //o->takeOwnership(tgt_dnd_so);
        }
        ImGui::EndDragDropTarget();
    }
}

void EditorResourceTree::imguiContextMenu(Editor* ed, const std::shared_ptr<ResourceNode>& node) {
    if (ImGui::BeginPopupContextItem()) {
        if(ImGui::MenuItem("Open")) {
            ed->tryOpenDocument(node);
            setSelected(node.get(), false);
        }
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
            renaming_node = (ResourceNode*)node.get();
        }
        if(ImGui::MenuItem("Delete")) {
            
        }
        ImGui::EndPopup();
    }
}

void EditorResourceTree::update(Editor* editor) {
    checkDirChanges();

    if(ImGui::Begin("Resource Tree")) {
        // TODO: ImGui::CalcTextSize()
        ImGui::BeginChild("ResourcePreview", ImVec2(0, 200));
        ImGui::Image(
            (ImTextureID)ImGui::GetIO().Fonts->TexID, ImVec2(150, 150), 
            ImVec2(0, 1), ImVec2(1, 0)
        );
        ImGui::TextWrapped("Select a resource to see it's description and preview here");
        ImGui::EndChild();

        ImGui::BeginChildFrame(ImGui::GetID("ResourceTreeTree"), ImVec2(0,0));
        std::function<void(const std::shared_ptr<ResourceNode>&)> imguiResourceTree;
        imguiResourceTree = [this, editor, &imguiResourceTree](const std::shared_ptr<ResourceNode>& node) {
            std::string node_label = node->getName();
            if(node->isLoaded()) {
                node_label += " [L]";
            }
            if(node->childCount()) {
                node_label = ICON_MDI_FOLDER " " + node_label;
                if(ImGui::TreeNodeEx(
                    (void*)node.get(),
                    ImGuiTreeNodeFlags_OpenOnDoubleClick |
                    ImGuiTreeNodeFlags_OpenOnArrow,
                    node_label.c_str()
                )) {
                    for(auto& kv : node->getChildren()) {
                        imguiResourceTree(kv.second);
                    }
                    ImGui::TreePop();
                }
            } else {
                bool selected = selected_node == node.get();
                if(selected && needs_autoscroll) {
                    needs_autoscroll = false;
                    ImGui::SetScrollY(ImGui::GetCursorScreenPos().y);
                }

                if(renaming_node == node.get()) {
                    char buf[256];
                    memset(buf, 0, 256);
                    memcpy(buf, node_label.c_str(), node_label.size());
                    ImGui::PushItemWidth(-1);
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0,0,0,1));
                    if(ImGui::InputText(MKSTR("###" << node.get()).c_str(), buf, 256, ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_EnterReturnsTrue)) {
                        renaming_node = 0;
                    }
                    ImGui::PopStyleColor();
                    ImGui::PopItemWidth();
                } else {
                    std::string node_name = node->getName();
                    std::string ext = node_name.substr(node_name.find_last_of("."));
                    std::string icon = getExtIconCode(ext.c_str());

                    if(ImGui::Selectable(MKSTR(icon << " " << node_label).c_str(), &selected, ImGuiSelectableFlags_AllowDoubleClick)) {
                        renaming_node = 0;
                        if(ImGui::IsMouseDoubleClicked(0)) {
                            editor->tryOpenDocument(node);                            
                        } else {
                        }
                        
                        setSelected(node.get(), false);
                    }
                    imguiDragDropSource(IMGUI_DND_RESOURCE, (void*)node.get(), node->getFullName());
                    imguiDragDropTarget(IMGUI_DND_RESOURCE);
                    imguiContextMenu(editor, node);
                }                
            }
        };

        for(auto& kv : gResourceTree.getRoot()->getChildren()) {
            imguiResourceTree(kv.second);
        }
        ImGui::Dummy(ImVec2(0, 200));
        ImGui::EndChildFrame();
    }
    ImGui::End();
}