#ifndef ECS_SUBSCENE_POSE_SYSTEM_HPP
#define ECS_SUBSCENE_POSE_SYSTEM_HPP

#include "../system.hpp"
#include "../attribs/base_attribs.hpp"

#include "../../resource/skeleton.hpp"

#include <assert.h>

typedef ecsTuple<ecsName, ecsOptional<ecsTranslation>, ecsOptional<ecsRotation>, ecsOptional<ecsScale>> ecsTupleAnimNode;

class ecsysSubScenePose : public ecsSystem<
    ecsTupleAnimNode
> {
private:
    std::shared_ptr<Skeleton> skeleton;
    std::vector<ecsTupleAnimNode*> nodes;
public:
    void onFit(ecsTupleAnimNode* node) {
        if(!skeleton) return;
        Skeleton::Bone* bone = skeleton->getBone(node->get<ecsName>()->name);
        if (!bone) {
          return;
        }
        nodes[bone->id] = node;
    }
    void onUnfit(ecsTupleAnimNode* node) {
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
            ecsTranslation* t = n->get_optional<ecsTranslation>();
            ecsRotation* r = n->get_optional<ecsRotation>();
            ecsScale* s = n->get_optional<ecsScale>();
            const AnimSample& sample = samples[i];

            if(t) t->setPosition(sample.t);
            if(r) r->setRotation(sample.r);
            if(s) s->setScale(sample.s);
        }
    }

    void setSkeleton(std::shared_ptr<Skeleton> skel) {
        skeleton = skel;
        if(!skeleton) return;
        //samples = skeleton->makePoseArray();
        nodes.resize(skeleton->boneCount());
        
        for(auto& t : get_array<ecsTupleAnimNode>()) {
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
