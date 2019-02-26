#ifndef SCENE_PLAYER_INFO_HPP
#define SCENE_PLAYER_INFO_HPP

#include "scene_component.hpp"

#include "../transform.hpp"

class ScenePlayerInfo : public SceneComponent {
public:
    Transform* character_transform = 0;
};

#endif
