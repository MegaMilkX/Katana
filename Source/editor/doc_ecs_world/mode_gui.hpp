#ifndef DOC_ECS_WORLD_MODE_GUI_HPP
#define DOC_ECS_WORLD_MODE_GUI_HPP

#include "mode.hpp"

void imguiEntityList_(
    ecsWorld* cur_world, 
    const std::vector<entity_id>& entities,
    entity_id& selected_ent
);

inline void imguiEntityListItem_(
    ecsEntityHandle hdl, 
    const std::string& name, 
    entity_id& selected_ent
) {
    bool selected = selected_ent == hdl.getId();

    ImVec4 text_color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
    if(hdl.getInheritedAttribBitmask()) {
        text_color = ImVec4(0.8f, 0.7f, 0.5f, 1.0f);
    }

    std::string string_id = MKSTR(name << "###" << hdl.getId());
    entity_id first_child_id = hdl.getWorld()->getFirstChild(hdl.getId());
    if(first_child_id == NULL_ENTITY) {
        ImGui::PushStyleColor(ImGuiCol_Text, text_color);
        if(ImGui::Selectable(string_id.c_str(), selected_ent == hdl.getId())) {
            selected_ent = hdl.getId();
        }
        ImGui::PopStyleColor();
        if(ImGui::BeginDragDropSource(0)) {
            ImGui::SetDragDropPayload("DND_ENTITY", &hdl, sizeof(hdl));
            ImGui::Text(string_id.c_str());
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_ENTITY")) {
                ecsEntityHandle hdlp = *(ecsEntityHandle*)payload->Data;
                hdl.getWorld()->setParent(hdl.getId(), hdlp.getId());
            }
            ImGui::EndDragDropTarget();
        }

        imguiEntityListItemContextMenu(string_id.c_str(), hdl, selected_ent);
    } else { 
        ImGuiTreeNodeFlags tree_node_flags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
        if(selected) {
            tree_node_flags |= ImGuiTreeNodeFlags_Selected;
        }
        ImGui::PushStyleColor(ImGuiCol_Text, text_color);
        bool open = ImGui::TreeNodeEx(MKSTR(name << "###" << hdl.getId()).c_str(), tree_node_flags);
        ImGui::PopStyleColor();
        if(ImGui::BeginDragDropSource(0)) {
            ImGui::SetDragDropPayload("DND_ENTITY", &hdl, sizeof(hdl));
            ImGui::Text(string_id.c_str());
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_ENTITY")) {
                ecsEntityHandle hdlp = *(ecsEntityHandle*)payload->Data;
                hdl.getWorld()->setParent(hdl.getId(), hdlp.getId());
            }
            ImGui::EndDragDropTarget();
        }
        imguiEntityListItemContextMenu(string_id.c_str(), hdl, selected_ent);
        if(ImGui::IsItemClicked(0)) {
            selected_ent = hdl.getId();
        }
        if(open) {
            std::vector<entity_id> children;
            entity_id child_id = first_child_id;
            while(child_id != NULL_ENTITY) {
                children.push_back(child_id);
                child_id = hdl.getWorld()->getNextSibling(child_id);
            }
            imguiEntityList_(hdl.getWorld(), children, selected_ent);
            ImGui::TreePop();
        }
    }
}

inline void imguiEntityList_(
    ecsWorld* cur_world, 
    const std::vector<entity_id>& entities,
    entity_id& selected_ent
) {
    std::vector<ecsEntityHandle> anon_entities;
    std::vector<ecsEntityHandle> unnamed_entities;
    std::vector<ecsEntityHandle> named_entities;
    
    for(auto e : entities) {
        ecsEntityHandle hdl(cur_world, e);
        ecsName* name = hdl.findAttrib<ecsName>();
        if(name) {
            if(!name->name.empty()) {
                named_entities.emplace_back(hdl);
            } else {
                unnamed_entities.emplace_back(hdl);
            }
        } else {
            anon_entities.emplace_back(hdl);
        }
    }

    std::sort(anon_entities.begin(), anon_entities.end(), [](const ecsEntityHandle& left, const ecsEntityHandle& right)->bool {
        return left.getId() < right.getId();
    });
    std::sort(unnamed_entities.begin(), unnamed_entities.end(), [](const ecsEntityHandle& left, const ecsEntityHandle& right)->bool {
        return left.getId() < right.getId();
    });
    std::sort(named_entities.begin(), named_entities.end(), [](const ecsEntityHandle& left, const ecsEntityHandle& right)->bool {
        return left.findAttrib<ecsName>()->name < right.findAttrib<ecsName>()->name;
    });

    for(int i = 0; i < named_entities.size(); ++i) {
        auto hdl = named_entities[i];
        std::string name = hdl.findAttrib<ecsName>()->name;
        imguiEntityListItem_(hdl, name, selected_ent);
    }
    for(int i = 0; i < unnamed_entities.size(); ++i) {
        auto hdl = unnamed_entities[i];
        imguiEntityListItem_(hdl, "[EMPTY_NAME]", selected_ent);
    }
    for(int i = 0; i < anon_entities.size(); ++i) {
        auto hdl = anon_entities[i];
        imguiEntityListItem_(hdl, "[ANONYMOUS]", selected_ent);
    }
}


class DocEcsWorldModeGui : public DocEcsWorldMode {
public:
    const char* getName() const override { return "GUI"; }

    void onMainWindow(DocEcsWorldState& state) override {
        if(state.gvp.begin()) {
        
        }
        state.gvp.end();
    }

    void onToolbox(DocEcsWorldState& state) override {
        if(ImGui::SmallButton(ICON_MDI_PLUS " Create")) {
            state.backupState();
            state.selected_ent = state.world->createEntity().getId();
            ecsEntityHandle h(state.world, state.selected_ent);
            h.getAttrib<ecsName>()->name = "New object";
        }
        ImGui::SameLine();
        if(ImGui::SmallButton(ICON_MDI_MINUS " Remove")) {
            state.backupState();
            state.world->removeEntity(state.selected_ent);
        }

        static std::string search_query;
        char buf[256];
        memset(buf, 0, 256);
        memcpy(buf, search_query.data(), search_query.size());
        if(ImGui::InputText(ICON_MDI_SEARCH_WEB, buf, 256)) {
            search_query = buf;
        }
        ImGui::PushItemWidth(-1);
        if(ImGui::ListBoxHeader("###OBJECT_LIST", ImVec2(0, 0))) {
            std::vector<entity_id> top_level_entities;
            auto& entities = state.world->getEntities();
            for(auto e : entities) {
                ecsEntityHandle hdl(state.world, e);
                if(state.world->getParent(e) == NULL_ENTITY) {
                    top_level_entities.emplace_back(hdl.getId());
                }
            }
            
            if(search_query.empty()) {
                imguiEntityList_(state.world, top_level_entities, state.selected_ent);
            } else {
                std::vector<entity_id> found_entities;
                auto& entities = state.world->getEntities();
                for(auto e : entities) {
                    ecsEntityHandle hdl(state.world, e);
                    ecsName* name = hdl.findAttrib<ecsName>();
                    if(!name) {
                        continue;
                    }
                    std::string a = name->name;
                    std::string b = search_query;
                    for(auto& c : a) c = std::toupper(c);
                    for(auto& c : b) c = std::toupper(c);
                    if(strncmp(a.c_str(), b.c_str(), b.size()) == 0) {
                        found_entities.emplace_back(hdl.getId());
                    }
                }
                imguiEntityList_(state.world, found_entities, state.selected_ent);
            }
            ImGui::ListBoxFooter();
        }
        ImGui::PopItemWidth();

        imguiEntityAttributeList(state);
    }
};


#endif
