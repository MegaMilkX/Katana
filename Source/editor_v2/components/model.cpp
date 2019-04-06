#include "model.hpp"

#include "../scene/controllers/render_controller.hpp"

CmModel::~CmModel() {
    getOwner()->getScene()->getEventMgr().unsubscribeAll(this);
    getOwner()->getScene()->getController<RenderController>()->_unregModel(this);
}

void CmModel::onCreate() {
    getOwner()->getScene()->getController<RenderController>()->_regModel(this);
    getOwner()->getScene()->getEventMgr().subscribe(this, EVT_OBJECT_REMOVED);
}