#ifndef BLEND_TREE_HPP
#define BLEND_TREE_HPP

#include "resource.h"

#include "../util/func_graph/node_graph.hpp"

#include "animation.hpp"
#include "skeleton.hpp"

#include "../util/imgui_helpers.hpp"

struct Pose {
    std::vector<AnimSample> samples;
    float speed = 1.0f;
};

class Blend3Job : public JobNode<Blend3Job> {
    RTTR_ENABLE(JobGraphNode)

    Pose pose;
public:
    void onInit() override {
        bind<Pose>(&pose);
    }
    void onInvoke() override {
        Pose& a = get<Pose>(0);
        Pose& b = get<Pose>(1);
        Pose& c = get<Pose>(2);
        float w = get<float>(3);
        if(w < .0f) {
            w = .0f;
        }
        if(w > 1.0f) {
            w = 1.0f;
        }
        const Pose* array[] = {
            &a, &b, &c
        };

        w *= 2.0f;

        int left_idx = w;
        if(left_idx == 2) {
            pose = c;
            return;
        }
        int right_idx = left_idx + 1;
        float lr_weight = w - (float)left_idx;

        const Pose* _a = array[left_idx];
        const Pose* _b = array[right_idx];

        pose.samples.resize(_a->samples.size());
        for(size_t i = 0; i < pose.samples.size(); ++i) {
            pose.samples[i].t = gfxm::lerp(_a->samples[i].t, _b->samples[i].t, lr_weight);
            pose.samples[i].r = gfxm::slerp(_a->samples[i].r, _b->samples[i].r, lr_weight);
            pose.samples[i].s = gfxm::lerp(_a->samples[i].s, _b->samples[i].s, lr_weight);
        }
        pose.speed = gfxm::lerp(_a->speed, _b->speed, lr_weight);
    }
};
STATIC_RUN(BLEND3_JOB) {
     rttr::registration::class_<Blend3Job>("Blend3JobNode")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}


class SingleAnimJob : public JobNode<SingleAnimJob> {
    RTTR_ENABLE(JobGraphNode)

    std::shared_ptr<Skeleton> skel;
    std::shared_ptr<Animation> anim;
    Pose pose;
public:
    SingleAnimJob() {
    }

    void setSkeleton(std::shared_ptr<Skeleton> skel) {
        this->skel = skel;
        if(!skel) return;
        pose.samples = skel->makePoseArray();
    }

    void onInit() override {
        bind<Pose>(&pose);
    }
    void onInvoke() override {
        if(!skel || !anim) return;

        std::vector<int32_t>& mapping = anim->getMapping(skel.get());
        anim->sample_remapped(pose.samples, .0f * anim->length, mapping);
        pose.speed = anim->fps / anim->length;
    }

    void onGui() override {
        imguiResourceTreeCombo("anim clip", anim, "anm", [](){

        });
    }
};
STATIC_RUN(SingleAnimJob) {
     rttr::registration::class_<SingleAnimJob>("SingleAnimJob")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}


class PoseResultJob : public JobNode<PoseResultJob> {
    RTTR_ENABLE(JobGraphNode)

public:
    Pose& getPose() {
        return get<Pose>(0);
    }

    void onInit() override {}
    void onInvoke() override {
        
    }
};
STATIC_RUN(PoseResultJob) {
     rttr::registration::class_<PoseResultJob>("PoseResultJob")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}


class FloatNode : public JobNode<FloatNode> {
    float v;
public:
    void onInit() {
        bind<float>(&v);
    }
    void onInvoke() {

    }

    void onGui() {
        ImGui::DragFloat("value", &v, .001f, .0f, 1.0f);
    }
};

class BlendTree : public Resource {
    RTTR_ENABLE(Resource)
    
    JobGraph jobGraph;
    std::set<SingleAnimJob*> anim_nodes;
    PoseResultJob* poseResult = 0;
    Pose pose;
public:
    BlendTree() {
        poseResult = new PoseResultJob();
        jobGraph.addNode(poseResult);
    }

    void clear();
    void copy(BlendTree* other);

    void setSkeleton(std::shared_ptr<Skeleton> skel);
    template<typename T>
    T* createNode() {
        auto node = new T();
        if(rttr::type::get<T>() == rttr::type::get<SingleAnimJob>()) {
            anim_nodes.insert(node);
        }
        jobGraph.addNode(node);
        return node;
    }

    const char* getWriteExtension() const override { return "blend_tree"; }

    Pose& getPoseData(float normal_cursor) {
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
