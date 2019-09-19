#include "camera.hpp"

#include "../util/octree.hpp"

static Octree octree;
static gfxm::aabb box1 = gfxm::aabb(
    gfxm::vec3(100, 100, 100),
    gfxm::vec3(103, 103, 105)
);
static gfxm::aabb box2 = gfxm::aabb(
    gfxm::vec3(-100, -100, -100),
    gfxm::vec3(-90, -90, -90)
);
static gfxm::aabb box3 = gfxm::aabb(
    gfxm::vec3(403, -40, 9),
    gfxm::vec3(407, 6, 15)
);

Camera::Camera() {
    octree.fit(box1);
    octree.fit(box2);
    octree.fit(box3);
}

gfxm::frustum Camera::getFrustum() {
    return gfxm::make_frustum(getProjection(1280, 720) * getView(), getView());
}


void Camera::debugDraw(DebugDraw* dd) {
    dd->frustum(getProjection(1280, 720), getView(), znear, zfar, gfxm::vec3(1,0,0));

    octree.debugDraw(*dd);

    dd->aabb(box1, gfxm::vec3(1,0,0));
    dd->aabb(box2, gfxm::vec3(1,0,0));
    dd->aabb(box3, gfxm::vec3(1,0,0));
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