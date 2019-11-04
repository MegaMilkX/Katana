#include "scene_graph_attribs.hpp"

#include "../systems/scene_graph.hpp"

void ecsTRS::dirty() {
    if(system) {
        system->setDirtyIndex(dirty_index);
    }
}