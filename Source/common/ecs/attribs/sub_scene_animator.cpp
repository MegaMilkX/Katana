#include "sub_scene_animator.hpp"

#include "../systems/animation_sys.hpp"


ecsSubSceneAnimator::ecsSubSceneAnimator()
: motion_ptr(&motion_lcl_) {

}

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
void ecsSubSceneAnimator::setMotion(std::shared_ptr<Motion> motion, bool dont_use_local_motion_object) {
    motion_ref = motion;
    motion_lcl_ = Motion();
    if(!motion_ref) {
        return;
    }

    if(dont_use_local_motion_object) {
        motion_ptr = motion_ref.get();
    } else {
        motion_ptr = &motion_lcl_;
    }

    dstream strm;
    motion_ref->serialize(strm);
    strm.jump(0);
    motion_lcl_.deserialize(strm, strm.bytes_available());

    if(skeleton) {
        motion_ptr->skeleton = skeleton;
        motion_ptr->rebuild();
    }

    if(tuple) {
        tuple->_skeletonChanged();
    }
}
Motion* ecsSubSceneAnimator::getLclMotion() {
    return motion_ptr;
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