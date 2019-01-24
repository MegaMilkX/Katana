#include "component.hpp"

#include "scene.hpp"

Scene* Component::getScene() {
    return scene_object->getScene();
}