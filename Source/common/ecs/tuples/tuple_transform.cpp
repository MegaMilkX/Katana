#include "tuple_transform.hpp"

#include "../systems/scene_graph.hpp"


void tupleTransform::onAddOptional(ecsParentTransform* p) {
    parent = system->get_tuple<tupleTransform>(p->parent_entity);
    if (parent) {
        parent->children.insert(this);
    }
}
void tupleTransform::onRemoveOptional(ecsParentTransform* p) {
    if (parent) {
        parent->children.erase(this);
    }
    parent = 0;
}