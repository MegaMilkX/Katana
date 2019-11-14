#ifndef ECS_SUBSCENE_POSE_SYSTEM_HPP
#define ECS_SUBSCENE_POSE_SYSTEM_HPP

#include "../system.hpp"
#include "../attribs/base_attribs.hpp"

#include "../../resource/skeleton.hpp"

#include <assert.h>

class ecsysSubScenePose : public ecsSystem<
    ecsTuple<ecsName, ecsWorldTransform>
> {
private:
    std::shared_ptr<Skeleton> skeleton;
    std::vector<ecsTuple<ecsName, ecsWorldTransform>*> nodes;
public:
    void onFit(ecsTuple<ecsName, ecsWorldTransform>* node) {
        if(!skeleton) return;
        Skeleton::Bone* bone = skeleton->getBone(node->get<ecsName>()->name);
        nodes[bone->id] = node;
    }
    void onUnfit(ecsTuple<ecsName, ecsWorldTransform>* node) {
        for(auto& n : nodes) { // No need to check for skeleton existence
            if(node == n) {
                n = 0;
            }
        }
    }

    void onUpdate() {

    }


    void setPose(const std::vector<AnimSample>& samples) {
        if(samples.size() != nodes.size()) {
            assert(false);
        }

        for(size_t i = 0; i < samples.size(); ++i) {
            auto n = nodes[i];
            if(!n) continue;
            ecsWorldTransform* t = n->get<ecsWorldTransform>();
            const AnimSample& sample = samples[i];

            t->transform = 
                gfxm::translate(gfxm::mat4(1.0f), sample.t)
                * gfxm::to_mat4(sample.r)
                * gfxm::scale(gfxm::mat4(1.0f), sample.s);
        }
    }

    void setSkeleton(std::shared_ptr<Skeleton> skel) {
        skeleton = skel;
        if(!skeleton) return;
        //samples = skeleton->makePoseArray();
        nodes.resize(skeleton->boneCount());
        
        for(auto& t : get_array<ecsTuple<ecsName, ecsWorldTransform>>()) {
            auto bone = skeleton->getBone(t->get<ecsName>()->name);
            if(bone) {
                nodes[bone->id] = t.get();
            }    
        }
    }
    const Skeleton* getSkeleton() const {
        return skeleton.get();
    }
};


#endif
