#ifndef ECS_ANIMATION_SYSTEM_HPP
#define ECS_ANIMATION_SYSTEM_HPP


#include "../system.hpp"
#include "sub_scene_pose.hpp"
#include "../attribs/base_attribs.hpp"
#include "../attribs/sub_scene_animator.hpp"

#include <assert.h>


class ecsTupleAnimatedSubScene : public ecsTuple<ecsSubSceneAnimator, ecsSubScene> {
public:
    ecsysSubScenePose* sysPose = 0;

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
        for(auto t : get_array<ecsTupleAnimatedSubScene>()) {
            auto animator = t->get<ecsSubSceneAnimator>();
            //if(!animator->motion) continue;
            if(!t->sysPose) continue;
            //animator->motion->update(1.0f/60.0f);
            /*
            const auto& samples = animator->motion->getSamples();
            if(samples.size() == 0) continue;
            t->sysPose->setPose(samples);*/
        }
    }
};


#endif
