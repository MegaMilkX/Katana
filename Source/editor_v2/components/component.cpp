#include "component.hpp"

#include "../scene/game_scene.hpp"

ObjectComponent::~ObjectComponent() {
    
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

bool ObjectComponent::serialize(std::ostream& out) {
    return false;
}
bool ObjectComponent::deserialize(std::istream& in, size_t sz) {
    return false;
}

IEditorComponentDesc* ObjectComponent::_newEditorDescriptor() {
    return new EditorComponentDesc<ObjectComponent>(this);
}