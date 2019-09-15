#include "editor_doc_texture2d.hpp"

#include "editor.hpp"


void EditorDocTexture2d::onGui(Editor* ed, float dt) {
    auto& texture = _resource;
    ImVec2 winMin = ImGui::GetWindowContentRegionMin();
    ImVec2 winMax = ImGui::GetWindowContentRegionMax();
    ImVec2 winSize = ImVec2(winMax - winMin);
    int tw = texture->Width();
    int th = texture->Height();
    if(tw > winSize.x) {
        float ratio = winSize.x / (float)tw;
        tw = winSize.x;
        th *= ratio;
    }
    if(th > winSize.y) {
        float ratio = winSize.y / (float)th;
        th = winSize.y;
        tw *= ratio;
    }
    ImGui::SetCursorPos((ImGui::GetWindowSize() - ImVec2(tw, th)) * 0.5f);
    if(texture) {
        ImGui::Image(
            (ImTextureID)texture->GetGlName(), 
            ImVec2(tw, th),
            ImVec2(0, 1), ImVec2(1, 0)
        );
    }
}