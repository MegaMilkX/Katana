#ifndef BLEND_TREE_HPP
#define BLEND_TREE_HPP

#include "resource.h"
#include "anim_primitive.hpp"

#include "blend_tree_nodes.hpp"

#include "../scene/game_scene.hpp"
#include "skeleton.hpp"
#include "../util/anim_blackboard.hpp"

#include "../util/object_pool.hpp"


class BlendTree : public Resource, public AnimatorBase, public JobGraphTpl<BlendTree> {
    RTTR_ENABLE(Resource)
    
    std::shared_ptr<Skeleton> skeleton;
    Pose pose;
    float cursor = .0f;
    Motion*         owner_motion = 0;

    std::map<std::string, int> value_indices;
    std::vector<float>         values;

public:
    // For editor purposes
    std::shared_ptr<GameScene> ref_object;
    std::shared_ptr<Skeleton> ref_skel;

    BlendTree() {
        addNode(new PoseResultJob());
    }

    ANIMATOR_TYPE getType() const override { return ANIMATOR_BLEND_TREE; }

    void clear() override;
    void copy(BlendTree* other);

    void setMotion(Motion* motion) override {
        owner_motion = motion;
        // TODO: Should not happen, but update value references anyway
    }
    Motion* getMotion() override {
        return owner_motion;
    }
    void setSkeleton(std::shared_ptr<Skeleton> skel) override;
    Skeleton* getSkeleton();

    int getValueIndex(const char* name);
    const char* getValueName(int idx);
    int declValue(const char* name);
    void removeValue(const char* name);
    void setValue(int idx, float val);
    float getValue(int idx);
    int valueCount();

    void setCursor(float cur) {
        cursor = cur;
        if(cursor > 1.0f) {
            cursor -= (int)cursor;
        }
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


    void compile() {
        value_indices.clear();
        values.clear();
        JobGraph::reinitNodes();
    }


    const char* getWriteExtension() const override { return "blend_tree"; }

    Pose& getPoseData(float normal_cursor) {
        run();
        return pose;
    }

    void update(float dt, std::vector<AnimSample>& samples) override {
        run();
        cursor += dt * pose.speed;
        if(cursor > 1.0f) {
            cursor -= 1.0f;
        }
        
        for(size_t i = 0; i < samples.size() && i < pose.samples.size(); ++i) {
            samples[i] = pose.samples[i];
        }
    }

    void         serialize(out_stream& out);
    bool         deserialize(in_stream& in, size_t sz);

    // Functions for node feedback
    void         _reportPose(const Pose& pose);
};
STATIC_RUN(BlendTree) {
    rttr::registration::class_<BlendTree>("BlendTree")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
