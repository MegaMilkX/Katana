#ifndef ECS_ACTOR_CHARACTER_HPP
#define ECS_ACTOR_CHARACTER_HPP

#include "actor.hpp"

#include "ecs/attribs/base_attribs.hpp"
#include "ecs/attribs/scene_graph_attribs.hpp"
#include "ecs/attribs/sub_scene_animator.hpp"
#include "ecs/attribs/transform.hpp"
#include "ecs/attribs/transform_tree.hpp"


class ecsACharacter : public ecsActor {
public:
    ecsName name;
    ecsTranslation translation;
    ecsRotation rotation;
    ecsScale scale;
    ecsWorldTransform world_transform;
    ecsLightOmni light_omni;

    ecsACharacter() {
        name.name = "ActorCharacter";
        light_omni.color = gfxm::vec3(1,1,1);
        light_omni.intensity = 10.0f;
        light_omni.radius = 10.0f;
    }
};


#endif
