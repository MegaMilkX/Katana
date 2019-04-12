#include "model.hpp"

#include "../scene/controllers/render_controller.hpp"

Model::~Model() {
    getOwner()->getScene()->getEventMgr().unsubscribeAll(this);
}

void Model::onCreate() {
    getOwner()->getScene()->getController<RenderController>();
    getOwner()->getScene()->getEventMgr().subscribe(this, EVT_OBJECT_REMOVED);
}