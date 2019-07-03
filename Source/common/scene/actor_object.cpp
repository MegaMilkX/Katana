#include "actor_object.hpp"

#include "controllers/actor_ctrl.hpp"

ActorObject::~ActorObject() {
    getScene()->getController<ActorCtrl>()->_unregActor(this);
}
void ActorObject::_onCreate() {
    getScene()->getController<ActorCtrl>()->_regActor(this);
    onCreate();
}