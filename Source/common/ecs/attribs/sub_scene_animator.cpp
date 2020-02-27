#include "sub_scene_animator.hpp"

#include "../systems/animation_sys.hpp"


void ecsSubSceneAnimator::setSkeleton(std::shared_ptr<Skeleton> skel) {
    skeleton = skel;
    if(motion_ref) {
        setMotion(motion_ref);
    }
    if(tuple) {
        tuple->_skeletonChanged();
    }
}
std::shared_ptr<Skeleton> ecsSubSceneAnimator::getSkeleton() {
    return skeleton;
}
void ecsSubSceneAnimator::setMotion(std::shared_ptr<Motion> motion) {
    motion_ref = motion;
    motion_lcl = Motion();
    if(!motion_ref) {
        return;
    }

    dstream strm;
    motion_ref->serialize(strm);
    strm.jump(0);
    motion_lcl.deserialize(strm, strm.bytes_available());

    if(skeleton) {
        motion_lcl.skeleton = skeleton;
        motion_lcl.rebuild();
    }

    if(tuple) {
        tuple->_skeletonChanged();
    }
}
Motion* ecsSubSceneAnimator::getLclMotion() {
    return &motion_lcl;
}

void ecsSubSceneAnimator::onGui(ecsWorld* world, entity_id ent) {
    imguiResourceTreeCombo("skeleton", skeleton, "skl", [this](){
        setSkeleton(skeleton);
    });
    imguiResourceTreeCombo("motion", motion_ref, "motion", [this](){
        setMotion(motion_ref);
    });
}

void ecsSubSceneAnimator::write(ecsWorldWriteCtx& out) {
    out.writeResource(skeleton);
    out.writeResource(motion_ref);
}
void ecsSubSceneAnimator::read(ecsWorldReadCtx& in) {
    skeleton = in.readResource<Skeleton>();
    motion_ref = in.readResource<Motion>();

    if(skeleton && motion_ref) {
        setSkeleton(skeleton);
        setMotion(motion_ref);
    }
}