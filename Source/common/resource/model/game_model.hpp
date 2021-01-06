#ifndef GAME_MODEL_HPP
#define GAME_MODEL_HPP

#include <vector>

#include "transform_graph.hpp"

struct GameModelObjectComponent {
    int type;
    void* ptr;
};

class GameModelObject {
    GameModel* model = 0;
    std::vector<GameModelObjectComponent> components;
public:

};

class GameModel {
    TransformGraph graph;
    std::vector<GameModelObject> objects;

public:

};


#endif
