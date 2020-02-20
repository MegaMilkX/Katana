#include "sub_scene_animator.hpp"

#include "../systems/animation_sys.hpp"


void ecsSubSceneAnimator::setSkeleton(std::shared_ptr<Skeleton> skel) {
    skeleton = skel;
    if(tuple) {
        tuple->_skeletonChanged();
    }
}
std::shared_ptr<Skeleton> ecsSubSceneAnimator::getSkeleton() {
    return skeleton;
}

void ecsSubSceneAnimator::onGui(ecsWorld* world, entity_id ent) {
    imguiResourceTreeCombo("skeleton", skeleton, "skl", [this](){
        setSkeleton(skeleton);
    });
    imguiResourceTreeCombo("motion", motion_ref, "motion", [this](){
        // TODO: Make a local copy
    });
}