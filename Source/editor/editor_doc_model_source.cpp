#include "editor_doc_model_source.hpp"

#include "../common/scene/game_scene.hpp"

EditorDocModelSource::EditorDocModelSource(ResourceNode* node)
: EditorDocument(node) {
    //gvp.camMode(GuiViewport::CAM_MODE::CAM_ORBIT);
    mdl_src = node->getResource<ModelSource>();
}

void EditorDocModelSource::onGui(Editor* ed) {
    ImVec2 winMin = ImGui::GetWindowContentRegionMin();
    ImVec2 winMax = ImGui::GetWindowContentRegionMax();
    ImVec2 winSize = ImVec2(winMax - winMin);
    float column0_ratio = 0.75f;

    ImGuiID dock_id = ImGui::GetID(getName().c_str());
    ImGui::DockSpace(dock_id);
    const std::string win_vp_name = MKSTR("viewport##" << getName());
    const std::string win_toolbar = MKSTR("toolbar##" << getName());
    if(first_use) {
        ImGuiID dsid_right = ImGui::DockBuilderSplitNode(dock_id, ImGuiDir_Right, 0.2f, NULL, &dock_id);

        ImGui::DockBuilderDockWindow(win_vp_name.c_str(), dock_id);
        ImGui::DockBuilderDockWindow(win_toolbar.c_str(), dsid_right);

        first_use = false;
    }

    ImGui::Begin(win_vp_name.c_str());
    gvp.draw(mdl_src->scene.get(), 0, gfxm::ivec2(0,0));
    ImGui::End();

    ImGui::Begin(win_toolbar.c_str());
    if(ImGui::Button("Unpack")) {
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
    ImGui::End();
}