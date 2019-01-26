#ifndef LIGHT_SOURCE_HPP
#define LIGHT_SOURCE_HPP

#include "component.hpp"
#include "gfxm.hpp"

class LightOmni : public Component {
    CLONEABLE
public:
    gfxm::vec3 color = gfxm::vec3(1.0f, 1.0f, 1.0f);

    virtual void serialize(std::ostream& out) {
    }
    virtual void deserialize(std::istream& in) {
    }
    virtual void _editorGui() {
        static bool alpha_preview = true;
        static bool alpha_half_preview = false;
        static bool options_menu = true;
        static bool hdr = false;
        int misc_flags = (hdr ? ImGuiColorEditFlags_HDR : 0) | (alpha_half_preview ? ImGuiColorEditFlags_AlphaPreviewHalf : (alpha_preview ? ImGuiColorEditFlags_AlphaPreview : 0)) | (options_menu ? 0 : ImGuiColorEditFlags_NoOptions);
        ImGui::ColorEdit3("Color", (float*)&color, misc_flags);
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
