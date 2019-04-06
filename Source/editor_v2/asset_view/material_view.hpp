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
        if(ImGui::ColorPicker3("tint", (float*)&mat->tint)) {}
        imguiResourceCombo(
            "albedo",
            mat->albedo, ".png"
        );
        imguiResourceCombo(
            "normal",
            mat->normal, ".png"
        );
        imguiResourceCombo(
            "metallic",
            mat->metallic, ".png"
        );
        imguiResourceCombo(
            "roughness",
            mat->roughness, ".png"
        );
    }
private:
    std::shared_ptr<Material> mat;
};