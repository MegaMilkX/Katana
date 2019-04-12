#ifndef GAME_SCENE_HPP
#define GAME_SCENE_HPP

#include <list>
#include <memory>

class GameObject {
public:

};

class SpatialObject : public GameObject {
public:

};

class StaticMesh : public SpatialObject {
public:

};

class SkinnedMesh : public SpatialObject {
public:
    
};

class GameScene {
public:
private:
    std::list<std::shared_ptr<GameObject>> objects;
};

#endif
