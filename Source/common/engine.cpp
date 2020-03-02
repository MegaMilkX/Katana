#include "engine.hpp"

#include "platform/platform.hpp"

#include "util/log.hpp"

#include "attributes/action_state_machine.hpp"
#include "attributes/animation_stack.hpp"
#include "attributes/audio_listener.hpp"
#include "attributes/audio_source.hpp"
#include "attributes/camera.hpp"
#include "attributes/collider.hpp"
#include "attributes/collision_listener.hpp"
#include "attributes/constraint_stack.hpp"
#include "attributes/light_source.hpp"
#include "attributes/model.hpp"
#include "attributes/render_environment.hpp"
#include "attributes/rigid_body.hpp"
#include "attributes/sample_attrib.hpp"
#include "attributes/skeleton_ref.hpp"
#include "attributes/tiled_room.hpp"
#include "attributes/voxel_field.hpp"

#include "attributes/actors/tps_camera.hpp"


#include "ecs/attribs/base_attribs.hpp"
#include "ecs/attribs/scene_graph_attribs.hpp"
#include "ecs/attribs/sub_scene_animator.hpp"
#include "ecs/attribs/transform.hpp"
#include "ecs/attribs/transform_tree.hpp"


static void initNodeAttribs() {
    REG_ATTRIB_INL(ActionStateMachine, ActionStateMachine, Animation)
    REG_ATTRIB_INL(AnimationStack, AnimationStack, Animation)
    REG_ATTRIB_INL(AudioListener, AudioListener, Audio)
    REG_ATTRIB_INL(AudioSource, AudioSource, Audio)
    REG_ATTRIB_INL(Camera, Camera, Rendering)
    REG_ATTRIB_INL(Collider, Collider, Physics)
    //REG_ATTRIB_INL(CollisionListener, CollisionListener, Physics)
    REG_ATTRIB_INL(ConstraintStack, ConstraintStack, Constraints)
    REG_ATTRIB_INL(OmniLight, OmniLight, Rendering)
    REG_ATTRIB_INL(DirLight, DirLight, Rendering)
    REG_ATTRIB_INL(Model, Model, Rendering)
    REG_ATTRIB_INL(RenderEnvironment, RenderEnvironment, Rendering)
    REG_ATTRIB_INL(RigidBody, RigidBody, Physics)
    REG_ATTRIB_INL(SampleAttrib, SampleAttrib, Sample)
    REG_ATTRIB_INL(SkeletonRef, SkeletonRef, Animation)
    REG_ATTRIB_INL(attribTiledRoomCtrl, TiledRoom, Test)
    REG_ATTRIB_INL(VoxelField, VoxelField, Rendering)

    REG_ATTRIB_INL(actorTpsCamera, TpsCameraActor, Actors)
}

static void initEcs() {
    regEcsAttrib<ecsName>("Name");
    regEcsAttrib<ecsSubScene>("SubScene");
    regEcsAttrib<ecsTagSubSceneRender>("TagSubSceneRender");
    regEcsAttrib<ecsTRS>("TRS");
    regEcsAttrib<ecsTranslation>("Translation");
    regEcsAttrib<ecsRotation>("Rotation");
    regEcsAttrib<ecsScale>("Scale");
    regEcsAttrib<ecsWorldTransform>("WorldTransform");
    regEcsAttrib<ecsParentTransform>("ParentTransform");
    regEcsAttrib<ecsTransform>("Transform");
    regEcsAttrib<ecsTransformTree>("TransformTree");
    regEcsAttrib<ecsVelocity>("Velocity");
    regEcsAttrib<ecsCollisionShape>("CollisionShape", "Collision");
    regEcsAttrib<ecsMass>("Mass", "Physics");
    regEcsAttrib<ecsMeshes>("Meshes", "Rendering");
    regEcsAttrib<ecsLightOmni>("LightOmni", "Rendering");
    regEcsAttrib<ecsSubSceneAnimator>("SubSceneAnimator");
}


bool katanaInit(PlatformStartupParams* params) {
    if(!platformInit(params)) {
        LOG_ERR("Failed to initialize platform");
        return false;
    }

    initNodeAttribs();
    initEcs();

    return true;
}

void katanaCleanup() {
    platformCleanup();
}