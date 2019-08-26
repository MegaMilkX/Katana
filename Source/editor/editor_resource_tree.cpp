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

#include "../common/gen/no_preview.png.h"

#include "preview_library.hpp"

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
        selected_branch_node = 0;
        selected_node = 0;

        if(FindNextChangeNotification(dwChangeHandle) == FALSE) {
            LOG_ERR("FindNextChangeNotification failed");
            return;
        }
        break;
    }
}


EditorResourceTree::EditorResourceTree() {
    initDirWatch(get_module_dir() + "/" + platformGetConfig().data_dir);
    selected_branch_node = gResourceTree.getRoot().get();
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

    std::shared_ptr<Texture2D> preview_tex = PreviewLibrary::get()->getPreviewPlaceholder();

    if(ImGui::Begin("Resource Tree")) {
        // TODO: ImGui::CalcTextSize()
        ImGui::BeginChild("ResourcePreview", ImVec2(0, 150));
        
        if(selected_node) {
            preview_tex = PreviewLibrary::get()->getPreview(selected_node->getFullName());
        }
        float win_width = ImGui::GetWindowContentRegionWidth();
        float win_height = 128;
        float size_ratio = win_width / (float)preview_tex->Width();
        float size_ratio_h = win_height / (float)preview_tex->Height();
        if(size_ratio_h < size_ratio) {
            size_ratio = size_ratio_h;
        }
        float cur_pos_x = ImGui::GetCursorPosX();
        cur_pos_x = (win_width - (preview_tex->Width() * size_ratio)) * 0.5f;
        ImGui::SetCursorPosX(cur_pos_x);
        ImGui::Image(
            (ImTextureID)preview_tex->GetGlName(), ImVec2(preview_tex->Width() * size_ratio, preview_tex->Height() * size_ratio), 
            ImVec2(0, 1), ImVec2(1, 0)
        );
        if(selected_node) { 
            ImGui::TextWrapped(selected_node->getName().c_str());
        }
        ImGui::EndChild();

        ImGui::BeginChildFrame(ImGui::GetID("ResourceTreeTree"), ImVec2(0,0));
        std::function<void(const std::shared_ptr<ResourceNode>&)> imguiResourceTree;
        imguiResourceTree = [this, editor, &imguiResourceTree](const std::shared_ptr<ResourceNode>& node) {
            std::string node_label = node->getName();
            if(node->isLoaded()) {
                node_label += " [L]";
            }

            std::vector<std::shared_ptr<ResourceNode>> sorted;
            for(auto& kv : node->getChildren()) {
                if(kv.second->childCount() == 0) {
                    continue;
                }
                sorted.emplace_back(kv.second);
            }
            std::sort(sorted.begin(), sorted.end(), [](const std::shared_ptr<ResourceNode>& a, const std::shared_ptr<ResourceNode>& b)->bool{
                if(a->childCount() && b->childCount()) {
                    return a->getName() < b->getName();
                } else if(a->childCount() == 0 && b->childCount() == 0) {
                    return a->getName() < b->getName();
                } else if(a->childCount() == 0) {
                    return false;
                } else {
                    return true;
                }
            });

            if(node->childCount()) {
                node_label = ICON_MDI_FOLDER " " + node_label;
                if(!sorted.empty()) {
                    ImGuiTreeNodeFlags tree_node_flags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
                    if(selected_branch_node == node.get()) {
                        tree_node_flags |= ImGuiTreeNodeFlags_Selected;
                    }
                    if(node == gResourceTree.getRoot()) {
                        tree_node_flags |= ImGuiTreeNodeFlags_DefaultOpen;
                        node_label = ICON_MDI_FOLDER " Resources";
                    }
                    bool tree_node_open = ImGui::TreeNodeEx(
                        (void*)node.get(),
                        tree_node_flags,
                        node_label.c_str()
                    );
                    if(ImGui::IsItemClicked(0)) {
                        selected_branch_node = node.get();
                    }
                    if(tree_node_open) {
                        for(auto& n : sorted) {
                            imguiResourceTree(n);
                        }
                        ImGui::TreePop();
                    }
                } else {
                    if(ImGui::Selectable(node_label.c_str(), selected_branch_node == node.get())) {
                        selected_branch_node = node.get();
                    }
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

        /*
        std::vector<std::shared_ptr<ResourceNode>> sorted;
        for(auto& kv : gResourceTree.getRoot()->getChildren()) {
            sorted.emplace_back(kv.second);
        }
        std::sort(sorted.begin(), sorted.end(), [](const std::shared_ptr<ResourceNode>& a, const std::shared_ptr<ResourceNode>& b)->bool{
            if(a->childCount() && b->childCount()) {
                return a->getName() < b->getName();
            } else if(a->childCount() == 0 && b->childCount() == 0) {
                return a->getName() < b->getName();
            } else if(a->childCount() == 0) {
                return false;
            } else {
                return true;
            }
        });
        for(auto& n : sorted) {
            imguiResourceTree(n);
        }*/
        imguiResourceTree(gResourceTree.getRoot());
        ImGui::Dummy(ImVec2(0, 200));
        ImGui::EndChildFrame();

        
    }
    ImGui::End();

    if(ImGui::Begin("Directory")) {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0,0,0,0));
        ImGui::BeginChildFrame(ImGui::GetID("ResourceTreeDir"), ImVec2(0,0));
        ImVec2 button_sz(64, 64);
        ImGuiStyle& style = ImGui::GetStyle();
        std::vector<std::shared_ptr<ResourceNode>> sorted_leaves;
        if(!selected_branch_node) {
            selected_branch_node = gResourceTree.getRoot().get();
        }
        for(auto& kv : selected_branch_node->getChildren()) {
            if(kv.second->childCount()) {
                continue;
            }
            sorted_leaves.emplace_back(kv.second);
        }
        int buttons_count = sorted_leaves.size();
        float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
        for(size_t i = 0; i < buttons_count; ++i) {
            auto node = sorted_leaves[i];
            std::string res_path = node->getFullName();
            std::string node_name = node->getName();
            auto tex = PreviewLibrary::get()->getPreview(res_path);
            float win_width = 128;
            float win_height = 128;
            float size_ratio = win_width / (float)preview_tex->Width();
            float size_ratio_h = win_height / (float)preview_tex->Height();
            if(size_ratio_h < size_ratio) {
                size_ratio = size_ratio_h;
            }

            ImGui::PushID(i);
            /*
            ImGui::Image(
                (ImTextureID)tex->GetGlName(), button_sz, 
                ImVec2(0, 1), ImVec2(1, 0)
            );*/
            ImVec2 textSize = ImGui::CalcTextSize(node_name.c_str(), 0, true, 64);
            ImVec2 itemSize(64, 64);
            itemSize.y += textSize.y;
            
            ImGuiWindow* window = ImGui::GetCurrentWindow();
            if (window->SkipItems)
                return;

            const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + itemSize);
            ImGui::ItemSize(bb);
            ImGui::ItemAdd(bb, ImGui::GetID(node_name.c_str()));

            ImVec2 itemRectMin = ImGui::GetItemRectMin();
            bool hovered = false;
            bool held = false;
            if(ImGui::ButtonBehavior(bb, ImGui::GetID(node_name.c_str()), &hovered, &held)) {
                setSelected(node.get(), false);
            }
            if(hovered) {
                if(ImGui::IsMouseDoubleClicked(0)) {
                    editor->tryOpenDocument(node);
                }
            }

            float last_button_x2 = ImGui::GetItemRectMax().x;
            float next_button_x2 = last_button_x2 + style.ItemSpacing.x + button_sz.x; // Expected position if next button was on same line
            if (i + 1 < buttons_count && next_button_x2 < window_visible_x2)
                ImGui::SameLine();
            ImGui::PopID();

            if(hovered || (selected_node == node.get())) {
                ImU32 col1 = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
                ImU32 col2 = ImGui::GetColorU32(ImGuiCol_Button);
                ImGui::GetWindowDrawList()->AddRectFilledMultiColor(
                    bb.Min, bb.Max, col2, col2, col1, col1
                );
            }

            float width = tex->Width();
            float height = tex->Height();
            ImVec2 img_sz(64, 64);
            ImVec2 img_pos = itemRectMin;
            img_sz = width > height ? ImVec2(64, 64 * (height / width)) : ImVec2(64 * (width / height), 64);
            img_pos.x += (64.0f - img_sz.x) * 0.5f;
            img_pos.y += (64.0f - img_sz.y) * 0.5f;

            ImGui::GetWindowDrawList()->AddImage(
                (ImTextureID)tex->GetGlName(), img_pos, img_pos + img_sz,
                ImVec2(0, 1), ImVec2(1, 0)
            );
            std::string ext = node_name.substr(node_name.find_last_of("."));
            std::string icon = getExtIconCode(ext.c_str());
            ImGui::RenderText(itemRectMin, icon.c_str());
            ImGui::RenderTextWrapped(itemRectMin + ImVec2(0, 64), sorted_leaves[i]->getName().c_str(), 0, 64);

            imguiDragDropSource(IMGUI_DND_RESOURCE, (void*)node.get(), node->getFullName());
            imguiDragDropTarget(IMGUI_DND_RESOURCE);
            imguiContextMenu(editor, node);
/*
            ImGui::Image(
                (ImTextureID)tex->GetGlName(), ImVec2(tex->Width() * size_ratio, tex->Height() * size_ratio), 
                ImVec2(0, 1), ImVec2(1, 0)
            );
            ImGui::Text(kv.second->getName().c_str());*/
        }
        ImGui::EndChildFrame();
        ImGui::PopStyleColor();
    }
    ImGui::End();
}