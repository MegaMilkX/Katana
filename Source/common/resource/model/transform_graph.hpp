#ifndef TRANSFORM_GRAPH_HPP
#define TRANSFORM_GRAPH_HPP

#include "../../gfxm.hpp"
#include "../../util/block_vector.hpp"
#include <string>


struct TransformNode {
    std::string name;

    int parent = -1;
    int next_sibling = -1;
    int first_child = -1;

    gfxm::vec3 translation;
    gfxm::quat rotation;
    gfxm::vec3 scale = gfxm::vec3(1,1,1);
    gfxm::mat4 local;
    gfxm::mat4 world;
};

class TransformGraph {
public:

};


#endif
