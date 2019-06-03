#ifndef SCENE_HPP
#define SCENE_HPP

#include "scene_node.hpp"

class Scene {
    SceneNode root_node;
public:
    SceneNode* getRoot() { return &root_node; }
};

#endif
