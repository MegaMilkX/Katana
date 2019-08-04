#ifndef LIGHT_SOURCE_HPP
#define LIGHT_SOURCE_HPP

#include "component.hpp"
#include "../../common/util/log.hpp"

class OmniLight : public Attribute {
    RTTR_ENABLE(Attribute)
public:
    gfxm::vec3 color = gfxm::vec3(1,1,1);
    float intensity = 1.0f;
    float radius = 5.0f;

    ~OmniLight();
    virtual void onCreate();

    virtual void copy(Attribute* other) {
        if(other->get_type() != get_type()) {
            LOG("Can't copy from " << other->get_type().get_name().to_string() << " to " <<
                get_type().get_name().to_string());
            return;
        }
        OmniLight* o = (OmniLight*)other;
        color = o->color;
        intensity = o->intensity;
        radius = o->radius;
    }
    virtual bool serialize(out_stream& out) {
        out.write(color);
        out.write(intensity);
        out.write(radius);
        return true;
    }
    virtual bool deserialize(in_stream& in, size_t sz) {
        color = in.read<gfxm::vec3>();
        intensity = in.read<float>();
        radius = in.read<float>();
        return true;
    }
    void onGui() {
        ImGui::ColorEdit3(MKSTR("color##" << this).c_str(), (float*)&color);
        ImGui::DragFloat(MKSTR("intensity##" << this).c_str(), &intensity, 0.001f);
        ImGui::DragFloat(MKSTR("radius##" << this).c_str(), &radius, 0.001f);
    }
    void onGizmo(GuiViewport& vp);
    virtual const char* getIconCode() const { return ICON_MDI_LIGHTBULB_ON; }
};
REG_ATTRIB(OmniLight, OmniLight, Rendering)

#endif
