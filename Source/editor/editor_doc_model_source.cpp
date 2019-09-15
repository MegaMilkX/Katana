#include "editor_doc_model_source.hpp"

#include "../common/scene/game_scene.hpp"


void EditorDocModelSource::onGui(Editor* ed, float dt) {
    auto& mdl_src = _resource;

    ImVec2 winMin = ImGui::GetWindowContentRegionMin();
    ImVec2 winMax = ImGui::GetWindowContentRegionMax();
    ImVec2 winSize = ImVec2(winMax - winMin);
    float column0_ratio = 0.75f;

    gvp.draw(mdl_src->scene.get(), 0, gfxm::ivec2(0,0));
}

void EditorDocModelSource::onGuiToolbox(Editor* ed) {
    auto& mdl_src = _resource;

    if(ImGui::Button("Import")) {
        mdl_src->unpack(get_module_dir() + "/" + platformGetConfig().data_dir);
    }

    ImGui::BeginChildFrame(ImGui::GetID("EditorDocModelSourceTree"), ImVec2(0,0));
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen;
    if(ImGui::TreeNodeEx("Meshes", flags)) {
        for(auto& m : mdl_src->meshes) {
            ImGui::Selectable(m->Name().c_str());
            //ImGui::TextColored(ImVec4(1,0,0,1), m->Name().c_str());
        }    
        ImGui::TreePop();
    }
    if(ImGui::TreeNodeEx("Animations", flags)) {
        for(auto& a : mdl_src->anims) {
            ImGui::Selectable(a->Name().c_str());
            //ImGui::TextColored(ImVec4(1,0,0,1), m->Name().c_str());
        }   
        ImGui::TreePop();
    }
    if(ImGui::TreeNodeEx("Materials", flags)) {
        for(auto& m : mdl_src->materials) {
            ImGui::TextColored(ImVec4(1,0,0,1), m->Name().c_str());
        } 
        ImGui::TreePop();
    }
    if(ImGui::TreeNodeEx("Textures", flags)) {
        for(auto& t : mdl_src->textures) {
            ImGui::TextColored(ImVec4(1,0,0,1), t->Name().c_str());
        }
        ImGui::TreePop();
    }
    ImGui::EndChildFrame();
}