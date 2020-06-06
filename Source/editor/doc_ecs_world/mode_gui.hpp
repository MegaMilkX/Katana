#ifndef DOC_ECS_WORLD_MODE_GUI_HPP
#define DOC_ECS_WORLD_MODE_GUI_HPP

#include "mode.hpp"


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
