#ifndef IMGUI_HELPERS_HPP
#define IMGUI_HELPERS_HPP

#include "../lib/imgui_wrap.hpp"

#include <string>
#include <memory>
#include <functional>
#include "../resource/data_registry.h"
#include "../resource/resource.h"
#include "../resource/resource_tree.hpp"

#include "../attributes/attribute.hpp"
#include "../scene/node.hpp"
#include "../scene/game_scene.hpp"

#include "../../editor/dialog_save.hpp"

#include "../util/materialdesign_icons.hpp"

#include "../resource/resource_desc_library.hpp"

#include "preview_library.hpp"


typedef void(*tryOpenDocumentFn_t)(const std::string&);
typedef void(*tryOpenDocumentFromPtrFn_t)(std::shared_ptr<Resource> res);

extern tryOpenDocumentFn_t gTryOpenDocumentFn;
extern tryOpenDocumentFromPtrFn_t gTryOpenDocumentFromPtrFn;


template<typename BASE_T>
inline void imguiHeapObjectCombo(
    const char* label,
    std::shared_ptr<BASE_T>& ptr,
    bool allow_null = true,
    std::function<void(void)> callback = nullptr
) {
    static const char* null_str = "<null>";
    auto derived_array = rttr::type::get<BASE_T>().get_derived_classes();
    if(ImGui::BeginCombo(label, ptr ? ptr->get_type().get_name().to_string().c_str() : null_str)) {
        if(allow_null) {
            if(ImGui::Selectable(null_str, !ptr)) {
                ptr = std::shared_ptr<BASE_T>();
            }
        }
        for(auto d : derived_array) {
            if(ImGui::Selectable(
                d.get_name().to_string().c_str(),
                ptr ? (d == ptr->get_type()) : false
                )
            ) {
                if(!d.is_valid()) {
                    LOG_WARN("Invalid type: " << d.get_name());
                    continue;
                }
                rttr::variant v = d.create();
                if(!v.is_valid() || !v.get_type().is_pointer()) {
                    LOG_WARN("Failed to create value of type " << d.get_name());
                    continue;
                }
                BASE_T* nptr = v.get_value<BASE_T*>();
                ptr.reset(nptr);
                callback();
            }
        }
        
        
        ImGui::EndCombo();
    }
}


template<typename T>
void imguiResourceTreeCombo(
    const char* label,
    std::shared_ptr<T>& res,
    const char* ext,
    std::function<void(void)> callback = nullptr
) {
    std::string current_name = "<null>";
    if(res) {
        current_name = res->Name();
    }
    if(current_name.empty()) {
        current_name = "<embedded>";
    }
    ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), label);
    
    if(ImGui::BeginCombo(MKSTR("###" << label).c_str(), current_name.c_str())) {
        std::set<const ResourceNode*> valid_nodes;
        std::function<bool(const std::shared_ptr<ResourceNode>&, const std::vector<std::string>&, std::set<const ResourceNode*>&)> walkNodes;
        walkNodes = [&walkNodes](const std::shared_ptr<ResourceNode>& node, const std::vector<std::string>& extensions,  std::set<const ResourceNode*>& valid_nodes)->bool {
            bool has_valid_child = false;
            for(auto& kv : node->getChildren()) {
                if(walkNodes(kv.second, extensions, valid_nodes)) {
                    has_valid_child = true;
                }
            }
            if(!has_valid_child) {
                for(int i = 0; i < extensions.size(); ++i) {
                    if(has_suffix(node->getName(), "." + extensions[i])) {
                        valid_nodes.insert(node.get());
                        return true;
                    }
                }
            } else {
                valid_nodes.insert(node.get());
                return true;
            }
            return false;
        };

        std::vector<std::string> tokens = split(ext, ',');
        walkNodes(gResourceTree.getRoot(), tokens, valid_nodes);

        std::function<void(const std::shared_ptr<ResourceNode>&, const std::set<const ResourceNode*>&)> imguiResourceTree;
        imguiResourceTree = [&callback, &res, &imguiResourceTree](const std::shared_ptr<ResourceNode>& node, const std::set<const ResourceNode*>& valid_nodes) {
            if(valid_nodes.find(node.get()) == valid_nodes.end()) {
                return;
            }
            std::string node_label = node->getName();
            if(node->isLoaded()) {
                node_label += " [L]";
            }
            if(node->childCount()) {
                if(ImGui::TreeNodeEx(
                    (void*)node.get(),
                    ImGuiTreeNodeFlags_OpenOnDoubleClick |
                    ImGuiTreeNodeFlags_OpenOnArrow,
                    node_label.c_str()
                )) {
                    for(auto& kv : node->getChildren()) {
                        imguiResourceTree(kv.second, valid_nodes);
                    }
                    ImGui::TreePop();
                }
            } else {
                auto tex = PreviewLibrary::get()->getPreview(node->getFullName(), time(0) /* TODO */);
                ImGui::Image(
                    (ImTextureID)tex->GetGlName(), 
                    ImVec2(ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight()),
                    ImVec2(0, 1), ImVec2(1, 0)
                ); ImGui::SameLine();
                bool selected = (res && res->Name() == node->getFullName());
                if(ImGui::Selectable(node_label.c_str(), selected)) {
                    res = node->getResource<T>();
                    if(callback) callback();
                }
            }
        };

        if(ImGui::Selectable("<null>", res == 0)) {
            res.reset();
            if(callback) callback();
        }
        for(auto& kv : gResourceTree.getRoot()->getChildren()) {
            imguiResourceTree(kv.second, valid_nodes);
        }

        ImGui::EndCombo();
    }
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_RESOURCE")) {
            ResourceNode* node = *(ResourceNode**)payload->Data;
            if(node) {
                auto r = node->getResource<T>();
                if(r) {
                    res = r;
                    if(callback) callback();
                }

            }
        }
        ImGui::EndDragDropTarget();
    }

    //ImGui::SameLine();

    if(res) {
        auto res_flags = ResourceDescLibrary::get()->getFlags(res->get_type());
        if((res_flags & ResourceDescLibrary::FLAG_VIEWABLE) == 0) {
            ImGui::TextDisabled(ICON_MDI_OPEN_IN_NEW);
        } else {
            if(ImGui::SmallButton(MKSTR(ICON_MDI_OPEN_IN_NEW << "###" << res.get()).c_str())) {
                if(res->Name().empty()) {
                    if(gTryOpenDocumentFromPtrFn) gTryOpenDocumentFromPtrFn(res);
                } else {
                    if(gTryOpenDocumentFn) gTryOpenDocumentFn(res->Name());
                }
            }
        }
        if (res_flags & ResourceDescLibrary::FLAG_WRITABLE) {
          if (res->Name().empty()) {
            ImGui::SameLine();
            if (ImGui::SmallButton(MKSTR(ICON_MDI_FLOPPY << "###" << res.get()).c_str())) {
              std::string save_path = dialogSave(ext);
              if (!save_path.empty()) {
                if (res->write_to_file(save_path)) {
                  res->Name(save_path);
                  if (callback) callback();
                }
              }
            }
          }
        }
        //ImGui::SameLine();
    } else {
        auto res_flags = ResourceDescLibrary::get()->getFlags(rttr::type::get<T>());
        if((res_flags & ResourceDescLibrary::FLAG_WRITABLE) == 0) {
            ImGui::TextDisabled(ICON_MDI_PLUS);
        } else {
            if(ImGui::SmallButton(MKSTR(ICON_MDI_PLUS << "###" << res.get()).c_str())) {
                res.reset(new T());
                if (callback) callback();
            }
        }
        //ImGui::SameLine();
    }
}

template<typename RES_T>
inline void imguiResourceCombo(
    const char* label,
    std::shared_ptr<RES_T>& res,
    const char* ext,
    std::function<void(void)> callback = nullptr
) {
    std::string name = "<null>";
    // TODO: FIX
    auto r_list = std::vector<std::string>(); //GlobalDataRegistry().makeList(ext);
    if(res) {
        name = res->Name();
    }
    if(ImGui::BeginCombo(label, name.c_str())) {
        if(ImGui::Selectable("<null>", !res)) {
            res = std::shared_ptr<RES_T>();
        }
        for(auto& rname : r_list) {
            if(ImGui::Selectable(rname.c_str(), strncmp(rname.c_str(), name.c_str(), name.size()) == 0)) {
                res = retrieve<RES_T>(rname);
                if(callback) callback();
            }
        }
        ImGui::EndCombo();
    }
}

template<typename COMP_T>
inline void imguiComponentCombo(
    const char* label,
    COMP_T*& c,
    GameScene* scene,
    std::function<void(void)> callback = nullptr
) {
    std::string name = "<null>";
    if(c) {
        name = c->getOwner()->getName();
    }

    if(ImGui::BeginCombo(label, name.c_str())) {
        auto& list = scene->getAllComponents<COMP_T>();
        if(ImGui::Selectable("<null>", c == 0)) {
            c = 0;
        }
        for(auto& comp : list) {
            if(ImGui::Selectable(comp->getOwner()->getName().c_str(), c == (COMP_T*)comp)) {
                c = (COMP_T*)comp;
                if(callback) callback();
            }
        }
        ImGui::EndCombo();
    }
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_OBJECT")) {
            ktNode* tgt_dnd_so = *(ktNode**)payload->Data;
            auto comp = tgt_dnd_so->find<COMP_T>();
            if(comp) {
                c = comp.get();
            }
        }
        ImGui::EndDragDropTarget();
    }
}

inline void imguiObjectCombo(
    const char* label,
    ktNode*& o,
    ktNode* root,
    std::function<void(void)> callback = nullptr
) {
    std::string name = "<null>";
    if(o) {
        name = o->getName();
    }
    if(ImGui::BeginCombo(label, name.c_str())) {
        if(ImGui::Selectable("<null>", o == 0)) {
            o = 0;
        }
        std::vector<ktNode*> list;
        root->getAllObjects(list);
        for(auto l : list) {
            if(ImGui::Selectable(l->getName().c_str(), o == l)) {
                o = l;
                if(callback) callback();
            }
        }
        ImGui::EndCombo();
    }
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_OBJECT")) {
            ktNode* tgt_dnd_so = *(ktNode**)payload->Data;
            o = tgt_dnd_so;
        }
        ImGui::EndDragDropTarget();
    }
}

#include "ecs/world.hpp"
enum GUI_ENTITY_LIST_FLAGS {
    GUI_ENTITY_LIST_FLAG_CONTEXT_MENU = 0x01
};
void saveTemplate(ecsEntityHandle hdl);
void imguiEntityListItemContextMenu(const char* string_id, ecsEntityHandle hdl, entity_id& selected_ent);
void imguiEntityListItem_(
    ecsEntityHandle hdl, 
    const std::string& name, 
    entity_id& selected_ent,
    int flags = GUI_ENTITY_LIST_FLAG_CONTEXT_MENU
);
void imguiEntityList_(
    ecsWorld* cur_world, 
    const std::vector<entity_id>& entities,
    entity_id& selected_ent,
    int flags = GUI_ENTITY_LIST_FLAG_CONTEXT_MENU
);
bool imguiEntityCombo(const char* label, ecsWorld* world, ecsEntityHandle& out);

namespace ImGui {

inline bool DragFloat3Autospeed(const char* label, float* v, float v_min = .0f, float v_max = .0f, const char* format = "%.3f", float power = 1.0f) {
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    int components = 3;

    ImGuiContext& g = *GImGui;
    bool value_changed = false;
    BeginGroup();
    PushID(label);
    PushMultiItemsWidths(components, CalcItemWidth());
    size_t type_size = sizeof(float);
    for (int i = 0; i < components; i++)
    {
        PushID(i);
        value_changed |= DragScalar(
            "##v", 
            ImGuiDataType_Float, 
            v, 
            std::max(std::abs(*(float*)v * 0.01f), 0.00001f), 
            &v_min, 
            &v_max, 
            format, 
            power
        );
        SameLine(0, g.Style.ItemInnerSpacing.x);
        PopID();
        PopItemWidth();
        v = (float*)((char*)v + type_size);
    }
    PopID();

    TextUnformatted(label, FindRenderedTextEnd(label));
    EndGroup();
    return value_changed;
}

}

#endif
