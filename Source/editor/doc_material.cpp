#include "doc_material.hpp"

#include "../common/util/imgui_helpers.hpp"

DocMaterial::DocMaterial() {

}
DocMaterial::DocMaterial(std::shared_ptr<ResourceNode>& node) {
    setResourceNode(node);
}

void DocMaterial::onGui(Editor* ed) {
    auto& mat = _resource;

    ImGui::ColorEdit3("tint", (float*)&mat->tint);

    if(mat->albedo) {
        ImGui::Image(
            (ImTextureID)mat->albedo->GetGlName(), ImVec2(100, 100), 
            ImVec2(0, 1), ImVec2(1, 0)
        );
    }
    imguiResourceTreeCombo(
        "albedo",
        mat->albedo, "png"
    );
    if(mat->normal) {
        ImGui::Image(
            (ImTextureID)mat->normal->GetGlName(), ImVec2(100, 100), 
            ImVec2(0, 1), ImVec2(1, 0)
        );
    }
    imguiResourceTreeCombo(
        "normal",
        mat->normal, "png"
    );
    if(mat->metallic) {
        ImGui::Image(
            (ImTextureID)mat->metallic->GetGlName(), ImVec2(100, 100), 
            ImVec2(0, 1), ImVec2(1, 0)
        );
    }
    imguiResourceTreeCombo(
        "metallic",
        mat->metallic, "png"
    );
    if(mat->roughness) {
        ImGui::Image(
            (ImTextureID)mat->roughness->GetGlName(), ImVec2(100, 100), 
            ImVec2(0, 1), ImVec2(1, 0)
        );
    }
    imguiResourceTreeCombo(
        "roughness",
        mat->roughness, "png"
    );
}