#ifndef LIGHT_SOURCE_HPP
#define LIGHT_SOURCE_HPP

#include "component.hpp"
#include "attrib_instantiable.hpp"
#include "../../common/util/log.hpp"

class OmniLight : public AttribInstantiable<OmniLight> {
    RTTR_ENABLE(Attribute)
public:
    ktProperty<gfxm::vec3> color = gfxm::vec3(1,1,1);
    ktProperty<float> intensity = 5.0f;
    ktProperty<float> radius = 5.0f;

    ~OmniLight();
    virtual void onCreate();

    virtual bool serialize(out_stream& out) {
        out.write(color.get());
        out.write(intensity.get());
        out.write(radius.get());
        return true;
    }
    virtual bool deserialize(in_stream& in, size_t sz) {
        color = in.read<gfxm::vec3>();
        intensity = in.read<float>();
        radius = in.read<float>();
        return true;
    }
    void onGui() {
        ImGui::ColorEdit3("color", (float*)&color.get());
        ImGui::DragFloat("intensity", &intensity.get(), 0.01f);
        ImGui::DragFloat("radius", &radius.get(), 0.01f);
    }
    void onGizmo(GuiViewport& vp);
    virtual const char* getIconCode() const { return ICON_MDI_LIGHTBULB_ON; }
};
STATIC_RUN(OmniLight) {
    rttr::registration::class_<OmniLight>("OmniLight")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
