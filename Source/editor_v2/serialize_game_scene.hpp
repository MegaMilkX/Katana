#ifndef SERIALIZE_GAME_SCENE_HPP
#define SERIALIZE_GAME_SCENE_HPP

#include "scene/game_scene.hpp"

bool serializeScene(const std::string& fname, GameScene* scn);
bool deserializeScene(const std::string& fname, GameScene* scn);

#endif
