#ifndef ECS_BASIC_ACTORS_HPP
#define ECS_BASIC_ACTORS_HPP

#include "actor.hpp"
#include "../attribs/base_attribs.hpp"

struct ecsANode : ecsActor {
    ecsName             name;
    ecsTranslation      translation;
    ecsRotation         rotation;
    ecsScale            scale;
    ecsWorldTransform   world_transform;
};

class ecsAComplexActor : ecsActor {
    std::set<entity_id> entities;
public:
    template<typename ACTOR_T>
    ecsActor* createActor();
    ecsEntityHandle createEntity();
};

class ecsAModel : public ecsAComplexActor {
public:

};


class ecsACharacter : public ecsAComplexActor {
    ecsAModel* model;
public:
    ecsACharacter() {
        model = createActor<ecsAModel>();
    }
};


#endif
