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

class ecsTplAnimator : public ecsTuple<ecsAnimator, ecsOptional<ecsTranslation>, ecsOptional<ecsWorldTransform>> {
public:

};

class ecsysAnimation : public ecsSystem<
    ecsTupleAnimatedSubScene,
    ecsTplAnimator
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
        for(int i = get_dirty_index<ecsTplAnimator>(); i < count<ecsTplAnimator>(); ++i) {
            auto t = get<ecsTplAnimator>(i);
            auto a = t->get<ecsAnimator>();
            if(a->skeleton) {
                a->sample_buffer = AnimSampleBuffer(a->skeleton.get());
                a->target_nodes.resize(a->skeleton->boneCount());
                for(int j = 0; j < a->skeleton->boneCount(); ++j) {
                    auto& b = a->skeleton->getBone(j);
                    auto chdl = a->getEntityHdl().findChild(b.name.c_str());
                    if(chdl.isValid()) {
                        a->target_nodes[j].t = chdl.findAttrib<ecsTranslation>();
                        a->target_nodes[j].r = chdl.findAttrib<ecsRotation>();
                        a->target_nodes[j].s = chdl.findAttrib<ecsScale>();
                    }
                }
            }
        }
        clear_dirty<ecsTplAnimator>();

        for(auto& t : get_array<ecsTplAnimator>()) {
            auto a = t->get<ecsAnimator>();
            if(!a->motion_ref) {
                continue;
            }
            if(!a->skeleton) {
                continue;
            }
            a->getLclMotion()->update(1.0f/60.0f, a->sample_buffer);
            for(int i = 0; i < a->sample_buffer.getSamples().size(); ++i) {
                auto& s = a->sample_buffer[i];
                auto& tn = a->target_nodes[i];
                if(tn.t) tn.t->setPosition(s.t);
                if(tn.r) tn.r->setRotation(s.r);
                if(tn.s) tn.s->setScale(s.s);
            }
        }

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
                rm_t = (world_trans->getTransform()) * gfxm::vec4(rm_t, .0f);
                translation->translate(rm_t);
            }
        }
    }
};


#endif
