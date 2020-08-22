#ifndef ECS_SYSTEM_ATTRIBS_HPP
#define ECS_SYSTEM_ATTRIBS_HPP


#include "../attribute.hpp"
#include "../../util/imgui_helpers.hpp"
#include <btBulletCollisionCommon.h>


struct ecsCollisionObject : public ecsAttrib<ecsCollisionObject, ecsAttribType::System> {
    btCollisionObject* collision_object = 0;
};


#endif
