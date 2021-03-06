#ifndef ECS_TYPES_HPP
#define ECS_TYPES_HPP


#include <stdint.h>


typedef uint32_t entity_id;
typedef int32_t  attrib_id;
typedef uint64_t archetype_mask_t;


enum ECS_SERIALIZED_RESOURCE_DATA_TYPE {
    ECS_RESOURCE_NULL,
    ECS_RESOURCE_EMBEDDED,
    ECS_RESOURCE_REFERENCE
};

static const entity_id      NULL_ENTITY             = -1;
const entity_id             ENTITY_ERROR            = (entity_id)-1;
const uint64_t              SERIALIZED_ENTITY_ERROR = (uint64_t)-1;

enum class ecsAttribType {
    Normal,
    System
};


#endif
