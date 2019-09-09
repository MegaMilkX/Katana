#ifndef BLEND_TREE_HPP
#define BLEND_TREE_HPP

#include "resource.h"

#include "blend_tree_nodes.hpp"


class BlendTree : public Resource {
    RTTR_ENABLE(Resource)
    
    JobGraph jobGraph;
    std::set<SingleAnimJob*> anim_nodes;
    PoseResultJob* poseResult = 0;
    Pose pose;
    float cursor = .0f;
public:
    BlendTree() {
        poseResult = new PoseResultJob();
        jobGraph.addNode(poseResult);
    }

    void clear();
    void copy(BlendTree* other);

    void setCursor(float cur) {
        cursor = cur;
        if(cursor > 1.0f) {
            cursor -= (int)cursor;
        }
    }
    float getCursor() const {
        return cursor;
    }

    void setSkeleton(std::shared_ptr<Skeleton> skel);
    template<typename T>
    T* createNode() {
        auto node = new T();
        if(rttr::type::get<T>() == rttr::type::get<SingleAnimJob>()) {
            anim_nodes.insert(node);
            ((SingleAnimJob*)node)->setBlendTree(this);
        }
        jobGraph.addNode(node);
        return node;
    }

    const char* getWriteExtension() const override { return "blend_tree"; }

    Pose& getPoseData(float normal_cursor) {
        jobGraph.run();
        
        if(!poseResult->isValid()) {
            return pose;
        } else {
            return poseResult->getPose();
        }
    }

    JobGraph& getGraph() {
        return jobGraph;
    }

    void         serialize(out_stream& out);
    bool         deserialize(in_stream& in, size_t sz);
};
STATIC_RUN(BlendTree) {
    rttr::registration::class_<BlendTree>("BlendTree")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
