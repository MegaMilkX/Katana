#include "gui_helpers.hpp"


void saveTemplate(ecsEntityHandle hdl) {
    std::string save_name;
    save_name = dialogSave("entity");
    if(!save_name.empty()) {
        fs_path p(get_module_dir() + "/" + platformGetConfig().data_dir);
        fs_path res_p(save_name);
        fs_path relative_path = p.relative(res_p);
        
        auto tpl = getResource<EntityTemplate>(relative_path.string(), true);
        tpl->getEntity().getWorld()->copyAttribs(tpl->getEntity().getId(), hdl);
        tpl->Name(relative_path.string());
        overwriteResourceFile(tpl);

        tpl->updateDerivedEntities();
        hdl.getWorld()->linkToTemplate(hdl.getId(), tpl);
    }
}

void imguiEntityListItemContextMenu(const char* string_id, ecsEntityHandle hdl, entity_id& selected_ent) {
    if(ImGui::BeginPopupContextItem(string_id)) {
        ImGui::Text(string_id);
        ImGui::Separator();
        if(ImGui::MenuItem("Duplicate")) {
            auto nhdl = hdl.getWorld()->createEntity();
            hdl.getWorld()->copyAttribs(nhdl.getId(), hdl);
            selected_ent = nhdl.getId();
        }
        if(hdl.getInheritedAttribBitmask()) {
            if(ImGui::MenuItem("Update template")) {
                hdl.getWorld()->updateTemplate(hdl.getId());
            }
        }
        if(ImGui::MenuItem("Save as template...")) {
            saveTemplate(hdl);
        }
        ImGui::Separator();
        if(ImGui::MenuItem("Delete")) {
            if(selected_ent == hdl.getId()) {
                selected_ent = 0;
            }
            hdl.remove();
        }
        ImGui::EndPopup();
    }
}

void imguiEntityListItem(
    ecsEntityHandle hdl, 
    const std::string& name, 
    entity_id& selected_ent, 
    const std::map<entity_id, std::vector<entity_id>>& child_map
) {
    bool selected = selected_ent == hdl.getId();

    ImVec4 text_color = ImGui::GetStyleColorVec4(ImGuiCol_Text);
    if(hdl.getInheritedAttribBitmask()) {
        text_color = ImVec4(0.8f, 0.7f, 0.5f, 1.0f);
    }

    auto it = child_map.find(hdl.getId());
    std::string string_id = MKSTR(name << "###" << hdl.getId());
    if(it == child_map.end()) {
        ImGui::PushStyleColor(ImGuiCol_Text, text_color);
        if(ImGui::Selectable(string_id.c_str(), selected_ent == hdl.getId())) {
            selected_ent = hdl.getId();
        }
        ImGui::PopStyleColor();
        imguiEntityListItemContextMenu(string_id.c_str(), hdl, selected_ent);
    } else { 
        ImGuiTreeNodeFlags tree_node_flags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
        if(selected) {
            tree_node_flags |= ImGuiTreeNodeFlags_Selected;
        }
        ImGui::PushStyleColor(ImGuiCol_Text, text_color);
        bool open = ImGui::TreeNodeEx(MKSTR(name << "###" << hdl.getId()).c_str(), tree_node_flags);
        ImGui::PopStyleColor();
        imguiEntityListItemContextMenu(string_id.c_str(), hdl, selected_ent);
        if(ImGui::IsItemClicked(0)) {
            selected_ent = hdl.getId();
        }
        if(open) {
            imguiEntityList(hdl.getWorld(), it->second, child_map, selected_ent);
            ImGui::TreePop();
        }
    }
}

void imguiEntityList(
    ecsWorld* cur_world, 
    const std::vector<entity_id>& entities,
    const std::map<entity_id, std::vector<entity_id>>& child_map,
    entity_id& selected_ent
) {
    std::vector<ecsEntityHandle> anon_entities;
    std::vector<ecsEntityHandle> unnamed_entities;
    std::vector<ecsEntityHandle> named_entities;
    
    for(auto e : entities) {
        ecsEntityHandle hdl(cur_world, e);
        ecsParentTransform* pt = hdl.findAttrib<ecsParentTransform>();
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
        imguiEntityListItem(hdl, name, selected_ent, child_map);
    }
    for(int i = 0; i < unnamed_entities.size(); ++i) {
        auto hdl = unnamed_entities[i];
        imguiEntityListItem(hdl, "[EMPTY_NAME]", selected_ent, child_map);
    }
    for(int i = 0; i < anon_entities.size(); ++i) {
        auto hdl = anon_entities[i];
        imguiEntityListItem(hdl, "[ANONYMOUS]", selected_ent, child_map);
    }
}


void imguiEntityAttributeList(DocEcsWorldState& state) {
    // TODO: Check selected entity validity
    if (state.world->getEntities().empty()) {
        return;
    }
    auto bitmaskInheritedAttribs = state.world->getInheritedAttribBitmask(state.selected_ent);
    ImGui::Text(MKSTR("UID: " << state.selected_ent).c_str());
    if(bitmaskInheritedAttribs) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Derived from template");

        if(ImGui::SmallButton("Reset")) {
            state.backupState();
            state.world->resetToTemplate(state.selected_ent);
        } ImGui::SameLine();
        if(ImGui::SmallButton("Update template")) {
            auto tpl = state.world->updateTemplate(state.selected_ent);
            if(tpl) {
                tpl->write_to_file(get_module_dir() + "/" + platformGetConfig().data_dir + "/" + tpl->Name());
            }
        }
    } else {
        if(ImGui::SmallButton("Make template")) {
            saveTemplate(ecsEntityHandle(state.world, state.selected_ent));
        }
    }

    ImGui::PushItemWidth(-1);
    if(ImGui::BeginCombo("###ADD_ATTRIBUTE", ICON_MDI_PLUS " Add attribute...")) {
        for(auto& it : getEcsAttribTypeLib().getTable()) {
            if(it.first == "") {
                for(auto& attr_id : it.second) {
                    auto inf = getEcsAttribTypeLib().get_info(attr_id);
                    if(ImGui::Selectable(inf->name.c_str())) {
                        state.backupState();
                        if(bitmaskInheritedAttribs & (1 << attr_id)) {
                            // Don't create attribute, just break inheritance
                            state.world->clearAttribInheritance(state.selected_ent, attr_id);
                        } else {
                            if(!state.world->getAttribPtr(state.selected_ent, attr_id)) {
                                state.world->createAttrib(state.selected_ent, attr_id);
                            }
                        }
                    }
                }
            } else {
                bool open = ImGui::TreeNode(it.first.c_str());
                if(open) {
                    for(auto& attr_id : it.second) {
                        auto inf = getEcsAttribTypeLib().get_info(attr_id);
                        if(ImGui::Selectable(inf->name.c_str())) {
                            state.backupState();
                            if(bitmaskInheritedAttribs & (1 << attr_id)) {
                                // Don't create attribute, just break inheritance
                                state.world->clearAttribInheritance(state.selected_ent, attr_id);
                            } else {
                                if(!state.world->getAttribPtr(state.selected_ent, attr_id)) {
                                    state.world->createAttrib(state.selected_ent, attr_id);
                                }
                            }
                        }
                    }

                    ImGui::TreePop();
                }
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    for(uint8_t i = 0; i < get_last_attrib_id() + 1; ++i) {
        auto attrib = state.world->getAttribPtr(state.selected_ent, i);
        if(!attrib) {
            continue;
        }
        auto inf = getEcsAttribTypeLib().get_info(attrib->get_id());
        const std::string& name = inf->name;
        bool exists = true;
        if(bitmaskInheritedAttribs & (1 << i)) {
            //ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);

            if(ImGui::CollapsingHeader(MKSTR( name ).c_str(), &exists, ImGuiTreeNodeFlags_DefaultOpen)) {
                attrib->onGui(state.world, state.selected_ent);
            }

            ImGui::PopStyleVar();
            ImGui::PopItemFlag();
            //ImGui::PopStyleColor();
        } else {
            if(ImGui::CollapsingHeader(MKSTR( name ).c_str(), &exists, ImGuiTreeNodeFlags_DefaultOpen)) {
                attrib->onGui(state.world, state.selected_ent);
            }
        }

        if(!exists) {
            state.world->removeAttrib(state.selected_ent, i);
        }
    }
}