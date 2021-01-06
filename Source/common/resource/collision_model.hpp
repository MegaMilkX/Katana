#ifndef COLLISION_MODEL_HPP
#define COLLISION_MODEL_HPP

#include "model/model.hpp"
#include <btBulletCollisionCommon.h>


struct CollisionObject {
    int                node;
    btCollisionObject* bt_object;
};

class CollisionModel {
    std::shared_ptr<Model_>         model;
    std::vector<CollisionObject>    collision_objects;

public:

};


#endif
