#ifndef BLEND_TREE_HPP
#define BLEND_TREE_HPP

#include "resource.h"
#include "anim_primitive.hpp"

#include "../util/func_graph/node_graph.hpp"

#include "../scene/game_scene.hpp"
#include "skeleton.hpp"
#include "../util/anim_blackboard.hpp"

#include "../util/object_pool.hpp"

struct Pose {
    AnimSampleBuffer        sample_buffer;
    float                   speed = 1.0f;
};

class BlendTree : public AnimatorBase, public JobGraphTpl<BlendTree> {    
    Motion*         owner_motion = 0;
    Pose            pose;
    float           prev_cursor = .0f;
    float           cursor = .0f;

public:
    BlendTree(Motion* motion);

    ANIMATOR_TYPE getType() const override { return ANIMATOR_BLEND_TREE; }

    void clear() override;
    void copy(BlendTree* other);

    Motion* getMotion() override;
    void rebuild() override;

    void setCursor(float cur);
    float getPrevCursor() const;
    float getCursor() const;

    template<typename T>
    T* createNode() {
        auto node = new T();
        addNode(node);
        return node;
    }

    Pose& getPoseData(float normal_cursor);

    void update(float dt, AnimSampleBuffer& sample_buffer) override;
    AnimSample   getRootMotion() override;

    void         write(out_stream& out) override;
    void         read(in_stream& in) override;

    // Functions for node feedback
    void         _reportPose(const Pose& pose);
};

#endif
