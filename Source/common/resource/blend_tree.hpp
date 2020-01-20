#ifndef BLEND_TREE_HPP
#define BLEND_TREE_HPP

#include "resource.h"

#include "blend_tree_nodes.hpp"

#include "../scene/game_scene.hpp"
#include "skeleton.hpp"
#include "../util/anim_blackboard.hpp"


class BlendTree : public Resource, public JobGraph {
    RTTR_ENABLE(Resource)
    
    std::shared_ptr<Skeleton> skeleton;
    Pose pose;
    float cursor = .0f;
    AnimBlackboard* blackboard = 0;

public:
    // For editor purposes
    std::shared_ptr<GameScene> ref_object;
    std::shared_ptr<Skeleton> ref_skel;

    BlendTree() {
        addNode(new PoseResultJob());
    }

    void clear() override;
    void copy(BlendTree* other);

    void setSkeleton(std::shared_ptr<Skeleton> skel);
    Skeleton* getSkeleton();
    void setBlackboard(AnimBlackboard* bb);

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

    const char* getWriteExtension() const override { return "blend_tree"; }

    Pose& getPoseData(float normal_cursor) {
        run();
        return pose;
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
