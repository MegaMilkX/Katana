#include "camera.hpp"


gfxm::frustum Camera::getFrustum() {
    return gfxm::make_frustum(getProjection(1280, 720) * getView(), getView());
}


void Camera::debugDraw(DebugDraw* dd) {
    dd->frustum(getProjection(1280, 720), getView(), znear, zfar, gfxm::vec3(1,0,0));
}

void Camera::onGui() {
    ImGui::DragFloat("fov", &fov, .001f, .0f, gfxm::pi);
    ImGui::DragFloat("znear", &znear, 0.01f, .0f, 100.0f);
    ImGui::DragFloat("zfar", &zfar, 0.01f, .0f, 10000.0f);
}
void Camera::onGizmo(GuiViewport& vp) {
    debugDraw(&vp.getDebugDraw());

    auto fr = getFrustum();
    if(gfxm::frustum_vs_point(fr, gfxm::vec3(0,0,-5))) {
        vp.getDebugDraw().point(gfxm::vec3(0,0,-5), gfxm::vec3(1,0,0));
    }
}