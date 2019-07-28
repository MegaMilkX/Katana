#include "editor_doc_scene.hpp"

#include "../common/scene/game_scene.hpp"

#include "../common/scene/controllers/render_controller.hpp"
#include "../common/scene/controllers/dynamics_ctrl.hpp"
#include "../common/scene/controllers/constraint_ctrl.hpp"
#include "../common/scene/controllers/audio_controller.hpp"
#include "../common/scene/controllers/anim_controller.hpp"

#include "editor.hpp"

EditorDocScene::EditorDocScene() {
    
}
EditorDocScene::EditorDocScene(std::shared_ptr<ResourceNode>& node) {
    setResourceNode(node);
    gvp.enableDebugDraw(true);
    
    bindActionPress("ALT", [this](){ 
        gvp.camMode(GuiViewport::CAM_ORBIT); 
    });
    bindActionRelease("ALT", [this](){ gvp.camMode(GuiViewport::CAM_PAN); });
}

void EditorDocScene::onGui (Editor* ed) {
    auto& scene = _resource;

    ImGuiID dock_id = ImGui::GetID(getName().c_str());
    ImGui::DockSpace(dock_id);

    const std::string win_vp_name = MKSTR("viewport##" << getName());
    const std::string win_scene_insp_name = MKSTR("Scene Inspector##" << getName());
    const std::string win_obj_insp_name = MKSTR("Object Inspector##" << getName());

    if(first_use) {
        ImGuiID dsid_right = ImGui::DockBuilderSplitNode(dock_id, ImGuiDir_Right, 0.2f, NULL, &dock_id);
        ImGuiID dsid_right_bottom = ImGui::DockBuilderSplitNode(dsid_right, ImGuiDir_Down, 0.5f, NULL, &dsid_right);

        ImGui::DockBuilderDockWindow(win_vp_name.c_str(), dock_id);
        ImGui::DockBuilderDockWindow(win_obj_insp_name.c_str(), dsid_right);
        ImGui::DockBuilderDockWindow(win_scene_insp_name.c_str(), dsid_right_bottom);

        first_use = false;
    }

    bool open = true;
    if(ImGui::Begin(win_vp_name.c_str(), &open, ImVec2(200, 200))) {
        if(ImGui::IsRootWindowOrAnyChildFocused()) {
            // TODO: Shouldn't be here
            ed->getResourceTree().setSelected(getNode());
            ed->setFocusedDocument(this);
        }

        gvp.draw(scene.get(), 0, gfxm::ivec2(0,0));
        ImGui::End();
    }

    scene_inspector.update(scene.get(), selected, win_scene_insp_name);
    object_inspector.update(scene.get(), selected, win_obj_insp_name);

    for(auto& o : selected.getAll()) {
        o->onGizmo(gvp);
    }
}