#include "ecs.hpp"

#include "ecs/attribs/base_attribs.hpp"
#include "ecs/attribs/scene_graph_attribs.hpp"
#include "ecs/attribs/sub_scene_animator.hpp"
#include "ecs/attribs/transform.hpp"
#include "ecs/attribs/transform_tree.hpp"


void ecsInit (void) {
    regEcsAttrib<ecsName>("Name");
    regEcsAttrib<ecsSubScene>("SubScene");
    regEcsAttrib<ecsTagSubSceneRender>("TagSubSceneRender");
    regEcsAttrib<ecsTranslation>("Translation", "Spatial");
    regEcsAttrib<ecsRotation>("Rotation", "Spatial");
    regEcsAttrib<ecsScale>("Scale", "Spatial");
    regEcsAttrib<ecsWorldTransform>("WorldTransform", "Spatial");
    regEcsAttrib<ecsParentTransform>("ParentTransform", "Spatial");
    regEcsAttrib<ecsVelocity>("Velocity");
    regEcsAttrib<ecsCollisionShape>("CollisionShape", "Collision");
    regEcsAttrib<ecsCollisionPlane>("CollisionPlane", "Collision");
    regEcsAttrib<ecsCollisionCache>("CollisionCache", "Collision");
    regEcsAttrib<ecsCollisionFilter>("CollisionFilter", "Collision");
    regEcsAttrib<ecsMass>("Mass", "Physics");
    regEcsAttrib<ecsMeshes>("Meshes", "Rendering");
    regEcsAttrib<ecsLightOmni>("LightOmni", "Rendering");

    regEcsAttrib<ecsGuiElement>("GuiElement", "GUI");
    regEcsAttrib<ecsGuiImage>("GuiImage", "GUI");

    regEcsAttrib<ecsSubSceneAnimator>("SubSceneAnimator");
    regEcsAttrib<ecsAudioListener>("AudioListener", "Audio");
    regEcsAttrib<ecsAudioSource>("AudioSource", "Audio");
}