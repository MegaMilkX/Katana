#include "ecs.hpp"

#include "ecs/attribs/base_attribs.hpp"
#include "ecs/attribs/scene_graph_attribs.hpp"
#include "ecs/attribs/sub_scene_animator.hpp"
#include "ecs/attribs/transform.hpp"
#include "ecs/attribs/transform_tree.hpp"

#include "actors/actor.hpp"
#include "actors/character.hpp"


void ecsInit (void) {
    regEcsAttrib<ecsName>("Name");
    regEcsAttrib<ecsSubScene>("SubScene");
    regEcsAttrib<ecsModel>("Model");
    regEcsAttrib<ecsTagSubSceneRender>("TagSubSceneRender");
    regEcsAttrib<ecsTranslation>("Translation", "Spatial");
    regEcsAttrib<ecsRotation>("Rotation", "Spatial");
    regEcsAttrib<ecsScale>("Scale", "Spatial");
    regEcsAttrib<ecsWorldTransform>("WorldTransform", "Spatial");
    regEcsAttrib<ecsVelocity>("Velocity");

    regEcsAttrib<ecsConstraint>("Constraint", "Constraints");
    regEcsAttrib<ecsIK>("IK", "Constraints");


    regEcsAttrib<ecsCollisionObject>("CollisionObject", "Collision");

    regEcsAttrib<ecsCollisionShape>("CollisionShape", "Collision");
    regEcsAttrib<ecsCollisionPlane>("CollisionPlane", "Collision");
    regEcsAttrib<ecsCollisionMesh>("CollisionMesh", "Collision");
    regEcsAttrib<ecsCollisionCache>("CollisionCache", "Collision");
    regEcsAttrib<ecsCollisionFilter>("CollisionFilter", "Collision");
    regEcsAttrib<ecsKinematicCharacter>("KinematicCharacter", "Collision");
    regEcsAttrib<ecsMass>("Mass", "Physics");
    regEcsAttrib<ecsMeshes>("Meshes", "Rendering");
    regEcsAttrib<ecsLightOmni>("LightOmni", "Rendering");
    regEcsAttrib<ecsLightDirect>("LightDirect", "Rendering");

    regEcsAttrib<ecsGuiElement>("GuiElement", "GUI");
    regEcsAttrib<ecsGuiText>("GuiText", "GUI");
    regEcsAttrib<ecsGuiImage>("GuiImage", "GUI");
    regEcsAttrib<ecsGuiAnchor>("GuiAnchor", "GUI");

    regEcsAttrib<ecsAnimator>("Animator", "Animation");
    regEcsAttrib<ecsSubSceneAnimator>("SubSceneAnimator", "Animation");

    regEcsAttrib<ecsAudioListener>("AudioListener", "Audio");
    regEcsAttrib<ecsAudioSource>("AudioSource", "Audio");

    regEcsAttrib<ecsBehavior>("Behavior");

    // TEST
    regEcsAttrib<ecsTestAttrib>("TestAttrib", "Test");


    // Actors
    ActorDescBuilder<ecsACharacter>("Character")
        .attrib(&ecsACharacter::name)
        .attrib(&ecsACharacter::translation)
        .attrib(&ecsACharacter::rotation)
        .attrib(&ecsACharacter::scale)
        .attrib(&ecsACharacter::world_transform)
        .attrib(&ecsACharacter::light_omni);
}