#ifndef ECS_TUPLE_TRANSFORM_HPP
#define ECS_TUPLE_TRANSFORM_HPP


#include "../system.hpp"
#include "../attribs/base_attribs.hpp"


class ecsysSceneGraph;
class tupleTransform : public ecsTuple<
    ecsOptional<ecsTranslation>,
    ecsOptional<ecsRotation>,
    ecsOptional<ecsScale>, 
    ecsWorldTransform
> {
public:
};


#endif
