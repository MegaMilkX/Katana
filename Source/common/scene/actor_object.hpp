#ifndef ACTOR_OBJECT_HPP
#define ACTOR_OBJECT_HPP

#include "game_object.hpp"

class ActorObject : public GameObject {
public:
    ~ActorObject();
    virtual void _onCreate();

    virtual void init() {}
    virtual void reset() {}
    virtual void update() = 0;
};

#endif
