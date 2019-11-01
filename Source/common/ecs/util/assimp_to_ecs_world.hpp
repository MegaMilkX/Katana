#ifndef ASSIMP_TO_ECS_WORLD_HPP
#define ASSIMP_TO_ECS_WORLD_HPP

#include "../systems/scene_graph.hpp"

bool assimpImportEcsScene(ecsysSceneGraph* world, const char* filename);


#endif
