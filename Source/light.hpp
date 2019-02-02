#ifndef LIGHT_SOURCE_HPP
#define LIGHT_SOURCE_HPP

#include "component.hpp"
#include "gfxm.hpp"

class LightOmni : public Component {
    CLONEABLE_AUTO
public:
    gfxm::vec3 color = gfxm::vec3(1.0f, 1.0f, 1.0f);
    float radius = 1.0f;
    float intensity = 1.0f;

    virtual void serialize(std::ostream& out) {
        out.write((char*)&color, sizeof(color));
        out.write((char*)&radius, sizeof(radius));
        out.write((char*)&intensity, sizeof(intensity));
    }
    virtual void deserialize(std::istream& in, size_t sz) {
        in.read((char*)&color, sizeof(color));
        in.read((char*)&radius, sizeof(radius));
        in.read((char*)&intensity, sizeof(intensity));
    }
    virtual void _editorGui() {
        static bool alpha_preview = true;
        static bool alpha_half_preview = false;
        static bool options_menu = true;
        static bool hdr = false;
        int misc_flags = (hdr ? ImGuiColorEditFlags_HDR : 0) | (alpha_half_preview ? ImGuiColorEditFlags_AlphaPreviewHalf : (alpha_preview ? ImGuiColorEditFlags_AlphaPreview : 0)) | (options_menu ? 0 : ImGuiColorEditFlags_NoOptions);
        ImGui::ColorEdit3("Color", (float*)&color, misc_flags);
        ImGui::DragFloat("Intensity", &intensity, 0.01f, 0.0f);
        ImGui::DragFloat("Radius", &radius, 0.01f, 0.0f);
    }
};
STATIC_RUN(LightOmni)
{
    rttr::registration::class_<LightOmni>("LightOmni")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
