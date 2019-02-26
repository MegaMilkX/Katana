#include "camera.hpp"
#include "scene.hpp"

Camera::~Camera() {
    if(getObject()->getScene()->getCurrentCamera() == this) {
        getObject()->getScene()->setCurrentCamera(0);
    }
}

const gfxm::mat4& Camera::getView() {
    view = gfxm::inverse(getObject()->get<Transform>()->getTransform());
    return view;
}

void Camera::makeCurrent() {
    getObject()->getScene()->setCurrentCamera(this);
}