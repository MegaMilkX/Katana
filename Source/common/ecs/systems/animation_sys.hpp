#ifndef ECS_ANIMATION_SYSTEM_HPP
#define ECS_ANIMATION_SYSTEM_HPP


#include "../system.hpp"
#include "sub_scene_pose.hpp"
#include "../attribs/base_attribs.hpp"
#include "../attribs/sub_scene_animator.hpp"

#include <assert.h>


class ecsTupleAnimatedSubScene : public ecsTuple<ecsSubSceneAnimator, ecsSubScene, ecsOptional<ecsTranslation>, ecsOptional<ecsWorldTransform>> {
public:
    ecsysSubScenePose* sysPose = 0;
    AnimSampleBuffer sample_buffer;

    void onAttribUpdate(ecsSubScene* ss) override {
        assert(ss->getWorld());
        sysPose = ss->getWorld()->getSystem<ecsysSubScenePose>();
        _skeletonChanged();
    }
    void onAttribUpdate(ecsSubSceneAnimator* a) {
        a->tuple = this;
        _skeletonChanged();
    }

    void _skeletonChanged() {
        auto animator = get<ecsSubSceneAnimator>();
        if(animator->getSkeleton()) {
            sysPose->setSkeleton(animator->getSkeleton());
            sample_buffer = AnimSampleBuffer(animator->getSkeleton().get());
        }
    }
};

class ecsysAnimation : public ecsSystem<
    ecsTupleAnimatedSubScene
> {
public:
    void onFit(ecsTupleAnimatedSubScene* t) {
        t->get<ecsSubSceneAnimator>()->tuple = t;
        ecsWorld* w = t->get<ecsSubScene>()->getWorld();
        ecsSubSceneAnimator* animator = t->get<ecsSubSceneAnimator>();
        assert(w);
        t->sysPose = w->getSystem<ecsysSubScenePose>();
        t->_skeletonChanged();
    }
    
    void onUpdate() {
        auto& arr = get_array<ecsTupleAnimatedSubScene>();
        for(auto t : arr) {
            auto animator = t->get<ecsSubSceneAnimator>();
            if(!animator->motion_ref) {
                continue;
            }
            if(!t->sysPose) {
                continue;
            }
            animator->getLclMotion()->update(1.0f/60.0f, t->sample_buffer);
            
            t->sysPose->setPose(t->sample_buffer.getSamples());

            ecsTranslation* translation = t->get_optional<ecsTranslation>();
            ecsWorldTransform* world_trans = t->get_optional<ecsWorldTransform>();

            //TODO: Root motion
            if (translation && world_trans) {
                gfxm::vec3 rm_t = t->sample_buffer.getRootMotionDelta().t;
                rm_t.y = .0f;
                rm_t = (world_trans->transform) * gfxm::vec4(rm_t, .0f);
                translation->translate(rm_t);
            }
        }
    }
};


#endif
