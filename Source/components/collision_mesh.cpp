#include "collision_shape.hpp"
#include "collider.hpp"

bool CollisionMesh::_editorGui(Collider* collider) {
    if(ImGui::Button("Make from model")) {
        makeFromModel(collider->getObject()->get<Model>());
        collider->refreshShape();
    }    
    return false;
}