#include "gui_helpers.hpp"





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