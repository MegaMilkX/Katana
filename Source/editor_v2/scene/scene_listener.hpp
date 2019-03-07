#ifndef SCENE_LISTENER_HPP
#define SCENE_LISTENER_HPP

#include "game_object.hpp"

#include "scene_events.hpp"

#include "../components/component.hpp"

class SceneListener {
public:
    virtual ~SceneListener() {}

    virtual void onSceneEvent(GameObject* sender, SCENE_EVENT e, rttr::variant payload) = 0;
};

#endif
