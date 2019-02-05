#include "component.hpp"

#include "scene.hpp"

Scene* Component::getScene() {
    return scene_object->getScene();
}

void Component::retriggerProbes(rttr::type t) {
    getScene()->retriggerProbes(t, this);
}