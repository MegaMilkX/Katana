#ifndef EDITOR_SCENE_HPP
#define EDITOR_SCENE_HPP

#include "editor_window.hpp"
#include "editor_viewport.hpp"

#include "../common_old/renderer.hpp"

class EditorScene : public EditorWindow {
public:
    EditorScene()
    : vp(&renderer) {}

    std::string Name() {
        return "Scene";
    }
    void Update() {
        if(ImGui::BeginMenuBar()) {
            if(ImGui::MenuItem("New viewport")) {}

            ImGui::EndMenuBar();
        }

        ImGuiID dockspace_id = ImGui::GetID("SceneDockspace");
        ImGui::DockSpace(dockspace_id);

        ImGui::SetNextWindowDockID(dockspace_id);
        if(ImGui::Begin("Viewport", 0, ImGuiWindowFlags_MenuBar)) {
            vp.Update();
        }
        ImGui::End();
    }

    void setScene(Scene* scn) {
        scene = scn;
        renderer.setScene(scene);
        vp.setScene(scn);
    }

    void setCursorPos(gfxm::ivec2 cursor) {
        vp.setCursorPos(cursor);
    }
private:
    EditorViewport vp;

    RendererPBR renderer;
    Scene* scene = 0;
};

#endif
