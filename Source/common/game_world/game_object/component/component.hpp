#ifndef KT_COMPONENT_HPP
#define KT_COMPONENT_HPP

#include <gfxm.hpp>
#include <resource/mesh.hpp>
class ktTransformComponent {
public:
    gfxm::vec3 translation;
    gfxm::quat rotation;
    gfxm::vec3 scale;
    gfxm::mat4 local;
    gfxm::mat4 world;
};
struct ktCTranslation {
    gfxm::vec3 translation;
};
struct ktCRotation {
    gfxm::quat rotation;
};
struct ktCScale {
    gfxm::vec3 scale;
};
struct ktCWorldTransform {
    gfxm::mat4 world_transform;
};
class ktActorNode;
struct ktCTransformRelations {
    ktActorNode* parent_node;
    ktActorNode* next_sibling;
    ktActorNode* first_child;
};
class ktLightOmniComponent {
public:
    gfxm::vec3 color;
    float intensity;
    float radius;
};
class ktMeshComponent {
    std::shared_ptr<Mesh> mesh;
};
class ktCollisionComponent {

};

class ktCoolComponent {
public:
    float my_value = 666.0f;
};
class ktVeryCoolComponent {
public:
    int cool_value = 9;
};


#endif
