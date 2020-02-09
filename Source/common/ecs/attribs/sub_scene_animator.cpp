#include "sub_scene_animator.hpp"

#include "../systems/animation_sys.hpp"


void ecsSubSceneAnimator::setSkeleton(std::shared_ptr<Skeleton> skel) {
    skeleton = skel;
    if(motion) {
        motion->setSkeleton(skeleton);
    }
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

    std::string currentMotionName = "<null>";
    if(motion) {
        currentMotionName = getMotionTypeName(motion->getMotionType());
    }
    if(ImGui::BeginCombo("motion", currentMotionName.c_str())) {
        if(ImGui::Selectable("SingleMotion", (motion && (motion->getMotionType() == MOTION_SINGLE)))) {
            motion.reset(new SingleMotion());
            motion->setSkeleton(skeleton);
        }
        if(ImGui::Selectable("BlendTree", (motion && (motion->getMotionType() == MOTION_BLENDTREE)))) {
            motion.reset(new BlendTreeMotion2());
            motion->setSkeleton(skeleton);
        }
        if(ImGui::Selectable("AnimFSM", (motion && (motion->getMotionType() == MOTION_ACTIONGRAPH)))) {
            motion.reset(new ActionGraphMotion());
            motion->setSkeleton(skeleton);
        }
        if(ImGui::Selectable("Layers", (motion && (motion->getMotionType() == MOTION_LAYERED)))) {
            motion.reset(new LayeredMotion());
            motion->setSkeleton(skeleton);
        }
        ImGui::EndCombo();
    }

    if(motion) {
        motion->onGui();
    }
}