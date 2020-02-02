#ifndef BLEND_TREE_MOTION_HPP
#define BLEND_TREE_MOTION_HPP

#include "motion.hpp"
#include "blend_tree.hpp"

#include "../util/imgui_helpers.hpp"

class BlendTreeMotion : public Motion {
    std::shared_ptr<BlendTree> tree;
    BlendTree local_tree;
    //std::set<SingleAnimJob*> single_anim_jobs;
    //PoseResultJob* poseResult;
public:
    void setBlendTree(std::shared_ptr<BlendTree> tree) {
        if(!tree){ 
            this->tree = 0;
            local_tree.clear();
        }
        this->tree = tree;
        local_tree.copy(tree.get());
        if(skeleton) {
            local_tree.setSkeleton(skeleton);
        } else {
            local_tree.setSkeleton(tree->ref_skel);
        }
        local_tree.compile();
        
        for(int i = 0; i < local_tree.valueCount(); ++i) {
            std::string name = local_tree.getValueName(i);
            // TODO
        }
    }

    BlendTree& getTree() {
        return local_tree;
    }

    void advance(float dt) override {
        if(!tree) return;


        Pose& pose = local_tree.getPoseData(cursor);
        cursor += dt * pose.speed;
        local_tree.setCursor(cursor);
        
        if(skeleton) {
            samples = pose.samples;
        }
        // TODO advance cursor
    }
    MOTION_TYPE getType() const override { return MOTION_BLEND_TREE; }

    void onSkeletonChanged() override {
        local_tree.setSkeleton(skeleton);
    }

    void onGui() override {
        imguiResourceTreeCombo("blend tree", tree, "blend_tree", [this](){
            setBlendTree(tree);
        });
    }
};

#endif
