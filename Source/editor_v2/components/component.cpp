#include "component.hpp"

#include "../scene/game_scene.hpp"

ObjectComponent::~ObjectComponent() {
    
}

rttr::type ObjectComponent::getRequiredOwnerType() {
    return rttr::type::get<GameObject>();
}

void ObjectComponent::onCreate() {}

void ObjectComponent::copy(ObjectComponent* other) {}

GameObject* ObjectComponent::getOwner() { return owner; }

bool ObjectComponent::buildAabb(gfxm::aabb& out) {
    out = gfxm::aabb(
        gfxm::vec3(.0f, .0f, .0f),
        gfxm::vec3(.0f, .0f, .0f)
    );
    return false;
}

bool ObjectComponent::serialize(out_stream& out) {
    return false;
}
bool ObjectComponent::deserialize(in_stream& in, size_t sz) {
    return false;
}

IEditorComponentDesc* ObjectComponent::_newEditorDescriptor() {
    return new EditorComponentDesc<ObjectComponent>(this);
}