#include "object_instance.hpp"

#include "game_scene.hpp"

#include "../common/util/imgui_helpers.hpp"

void ktObjectInstance::setScene(std::shared_ptr<GameScene> scene) {
    this->scene = scene;

    // TODO:
    copy(scene.get(), OBJECT_FLAG_TRANSIENT);
}
std::shared_ptr<GameScene> ktObjectInstance::getScene() const {
    return scene;
}

void ktObjectInstance::onGui() {
    imguiResourceTreeCombo("source", scene, "so", [this](){
        setScene(scene);
    });

    ktNode::onGui();
}