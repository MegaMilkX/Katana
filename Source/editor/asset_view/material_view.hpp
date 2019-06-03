#include "asset_view.hpp"

#include "../../common/resource/material.hpp"
#include "../../common/util/imgui_helpers.hpp"

class MaterialView : public AssetView {
public:
    MaterialView(std::shared_ptr<Material> mat)
    : mat(mat) {

    }

    virtual void onGui() {
        if(!mat) return;
        ImGui::ColorEdit3("tint", (float*)&mat->tint);

        if(mat->albedo) {
            ImGui::Image(
                (ImTextureID)mat->albedo->GetGlName(), ImVec2(100, 100), 
                ImVec2(0, 1), ImVec2(1, 0)
            );
        }
        imguiResourceCombo(
            "albedo",
            mat->albedo, ".png"
        );
        if(mat->normal) {
            ImGui::Image(
                (ImTextureID)mat->normal->GetGlName(), ImVec2(100, 100), 
                ImVec2(0, 1), ImVec2(1, 0)
            );
        }
        imguiResourceCombo(
            "normal",
            mat->normal, ".png"
        );
        if(mat->metallic) {
            ImGui::Image(
                (ImTextureID)mat->metallic->GetGlName(), ImVec2(100, 100), 
                ImVec2(0, 1), ImVec2(1, 0)
            );
        }
        imguiResourceCombo(
            "metallic",
            mat->metallic, ".png"
        );
        if(mat->roughness) {
            ImGui::Image(
                (ImTextureID)mat->roughness->GetGlName(), ImVec2(100, 100), 
                ImVec2(0, 1), ImVec2(1, 0)
            );
        }
        imguiResourceCombo(
            "roughness",
            mat->roughness, ".png"
        );
    }
private:
    std::shared_ptr<Material> mat;
};