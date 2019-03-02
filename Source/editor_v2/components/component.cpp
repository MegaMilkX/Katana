#include "component.hpp"

#include "../scene/game_scene.hpp"

ObjectComponent::~ObjectComponent() {
    getOwner()->getScene()->getEventMgr().post(
        getOwner(),
        EVT_COMPONENT_REMOVED,
        get_type()
    );
}