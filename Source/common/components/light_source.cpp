#include "light_source.hpp"

#include "../scene/node.hpp"

OmniLight::~OmniLight() {
}
void OmniLight::onCreate() {}

void OmniLight::onGizmo(GuiViewport& vp) {
    gfxm::mat3 mat_a = gfxm::to_mat3(gfxm::angle_axis(gfxm::pi / 2.0f, gfxm::vec3(1.0f, 0.0, 0.0f)));
    gfxm::mat3 mat_b = gfxm::to_mat3(gfxm::angle_axis(gfxm::pi / 2.0f, gfxm::vec3(0.0f, 0.0, 1.0f)));
    vp.getDebugDraw().circle(getOwner()->getTransform()->getWorldPosition(), radius, color);
    vp.getDebugDraw().circle(getOwner()->getTransform()->getWorldPosition(), radius, color, mat_a);
    vp.getDebugDraw().circle(getOwner()->getTransform()->getWorldPosition(), radius, color, mat_b);
}