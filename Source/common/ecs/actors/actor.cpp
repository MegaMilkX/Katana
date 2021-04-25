#include "actor.hpp"

static actor_desc_map_t actor_desc_map;

actor_desc_map_t& getActorDescMap() {
    return actor_desc_map;
}