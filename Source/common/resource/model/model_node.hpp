#ifndef MODEL_NODE_HPP
#define MODEL_NODE_HPP

#include <string>
#include <vector>
#include "../../gfxm.hpp"

class ModelNode {
public:
    std::string name;

    int parent = -1;
    std::vector<int> children;

    gfxm::vec3 translation;
    gfxm::quat rotation;
    gfxm::vec3 scale = gfxm::vec3(1,1,1);
    gfxm::mat4 local;
    gfxm::mat4 world;
};

#endif
