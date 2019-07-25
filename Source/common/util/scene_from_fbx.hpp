#ifndef SCENE_OBJECT_FROM_FBX_2_HPP
#define SCENE_OBJECT_FROM_FBX_2_HPP

#include <string>
#include <vector>

class GameScene;

bool sceneFromFbx(const std::vector<char>& buffer, GameScene* scn, const std::string& filename = "");
bool sceneFromFbx(const std::string& filename, GameScene* scn);

#endif
