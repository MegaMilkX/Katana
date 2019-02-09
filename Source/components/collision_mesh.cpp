#include "collision_shape.hpp"

bool CollisionMesh::_editorGui(BaseCollisionComponent* collider) {
    if(ImGui::Button("Make from model")) {
        makeFromModel(collider->getObject()->get<Model>());
        collider->refreshShape();
    }    
    return false;
}