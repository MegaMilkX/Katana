#include "object_instance.hpp"

#include "game_scene.hpp"

void ktObjectInstance::setScene(std::shared_ptr<GameScene> scene) {
    this->scene = scene;

    // TODO:
    copy(scene.get());
}
GameScene* ktObjectInstance::getScene() const {
    return scene.get();
}