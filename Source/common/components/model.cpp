#include "model.hpp"

#include "../scene/controllers/render_controller.hpp"

Model::~Model() {
}

void Model::onCreate() {
    getOwner()->getScene()->getController<RenderController>();
}