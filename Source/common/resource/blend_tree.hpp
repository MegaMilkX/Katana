#ifndef BLEND_TREE_HPP
#define BLEND_TREE_HPP

#include "resource.h"
#include "anim_primitive.hpp"

#include "blend_tree_nodes.hpp"

#include "../scene/game_scene.hpp"
#include "skeleton.hpp"
#include "../util/anim_blackboard.hpp"

#include "../util/object_pool.hpp"


class BlendTree : public AnimatorBase, public JobGraphTpl<BlendTree> {    
    Motion*         owner_motion = 0;
    Pose            pose;
    float           prev_cursor = .0f;
    float           cursor = .0f;

public:
    BlendTree(Motion* motion)
    : owner_motion(motion) {
        addNode(new PoseResultJob());
    }

    ANIMATOR_TYPE getType() const override { return ANIMATOR_BLEND_TREE; }

    void clear() override;
    void copy(BlendTree* other);

    Motion* getMotion() override {
        return owner_motion;
    }
    void rebuild() override {
        JobGraph::reinitNodes();
    }

    void setCursor(float cur) {
        prev_cursor = cursor;
        cursor = cur;
        if(cursor > 1.0f) {
            cursor -= (int)cursor;
        }
    }
    float getPrevCursor() const {
        return prev_cursor;
    }
    float getCursor() const {
        return cursor;
    }

    template<typename T>
    T* createNode() {
        auto node = new T();
        addNode(node);
        return node;
    }

    Pose& getPoseData(float normal_cursor) {
        run();
        return pose;
    }

    void update(float dt, AnimSampleBuffer& sample_buffer) override {
        run();
        prev_cursor = cursor;
        cursor += dt * pose.speed;
        if(cursor > 1.0f) {
            cursor -= 1.0f;
        }
        
        for(size_t i = 0; i < sample_buffer.sampleCount() && i < pose.sample_buffer.sampleCount(); ++i) {
            sample_buffer[i] = pose.sample_buffer[i];
        }
        sample_buffer.getRootMotionDelta() = pose.sample_buffer.getRootMotionDelta();
    }
    AnimSample   getRootMotion() override {
        AnimSample sample;

        

        return sample;
    }

    void         write(out_stream& out) override;
    void         read(in_stream& in) override;

    // Functions for node feedback
    void         _reportPose(const Pose& pose);
};

#endif
