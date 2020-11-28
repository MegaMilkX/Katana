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

class ecsTplAnimatedModel : public ecsTuple<ecsAnimator, ecsOptional<ecsTranslation>, ecsOptional<ecsWorldTransform>> {
public:

};

class ecsysAnimation : public ecsSystem<
    ecsTupleAnimatedSubScene,
    ecsTplAnimatedModel
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
        for(int i = get_dirty_index<ecsTplAnimatedModel>(); i < count<ecsTplAnimatedModel>(); ++i) {
            auto t = get<ecsTplAnimatedModel>(i);
            auto a = t->get<ecsAnimator>();
            if(a->skeleton) {
                a->sample_buffer = AnimSampleBuffer(a->skeleton.get());
                a->model_target_nodes.resize(a->skeleton->boneCount());
                for(int j = 0; j < a->skeleton->boneCount(); ++j) {
                    auto& b = a->skeleton->getBone(j);
                    auto node_id = t->get<ecsModel>()->model->findNodeId(b.name.c_str());
                    a->model_target_nodes[j] = node_id;    
                }
            }
        }
        clear_dirty<ecsTplAnimatedModel>();

        for(auto& t : get_array<ecsTplAnimatedModel>()) {
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
                auto tgt_id = a->model_target_nodes[i];
                t->get<ecsModel>()->model->nodes[tgt_id].translation = s.t;
                t->get<ecsModel>()->model->nodes[tgt_id].rotation = s.r;
                t->get<ecsModel>()->model->nodes[tgt_id].scale = s.s;
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
            animator->sample_buffer.getRootMotionDelta().t = gfxm::vec3(0,0,0);
            animator->sample_buffer.getRootMotionDelta().r = gfxm::quat(0,0,0,1);
            animator->getLclMotion()->update(1.0f/60.0f, animator->sample_buffer);
            
            t->sysPose->setPose(animator->sample_buffer.getSamples());

            ecsTranslation* translation = t->get_optional<ecsTranslation>();
            ecsWorldTransform* world_trans = t->get_optional<ecsWorldTransform>();

            //TODO: Root motion
            if (translation && world_trans) {
                gfxm::vec3 rm_t = animator->sample_buffer.getRootMotionDelta().t;
                rm_t.y = .0f;
                rm_t = (world_trans->getTransform()) * gfxm::vec4(rm_t, .0f);
                translation->translate(rm_t);
            }
        }
    }
};


#endif
