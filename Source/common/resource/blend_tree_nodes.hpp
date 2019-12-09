#ifndef BLEND_TREE_NODES_HPP
#define BLEND_TREE_NODES_HPP

#include "../util/func_graph/node_graph.hpp"
#include "animation.hpp"
#include "skeleton.hpp"

#include "../util/imgui_helpers.hpp"

class BlendTree;

struct Pose {
    std::vector<AnimSample> samples;
    float speed = 1.0f;
};

class Blend3Job : public JobNode<Blend3Job> {
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


class SingleAnimJob : public JobNode<SingleAnimJob> {
    // TODO: Remove?
    std::shared_ptr<Skeleton> skel;

    std::shared_ptr<Animation> anim;
    Pose pose;

    BlendTree* blendTree = 0;

public:
    SingleAnimJob();

    void setBlendTree(BlendTree* bt);

    void setSkeleton(std::shared_ptr<Skeleton> skel);

    void onInit() override;
    void onInvoke() override;

    void onGui() override;
};


class PoseResultJob : public JobNode<PoseResultJob> {
public:
    Pose& getPose() {
        return get<Pose>(0);
    }

    void onInit() override {}
    void onInvoke() override {
        
    }
};


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



#endif
