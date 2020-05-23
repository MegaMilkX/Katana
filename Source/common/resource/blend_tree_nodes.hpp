#ifndef BLEND_TREE_NODES_HPP
#define BLEND_TREE_NODES_HPP

#include "../util/func_graph/node_graph.hpp"
#include "animation.hpp"
#include "skeleton.hpp"

#include "../util/imgui_helpers.hpp"

class BlendTree;

struct Pose {
    AnimSampleBuffer        sample_buffer;
    float                   speed = 1.0f;
};

class Blend3Job : public JobNode<Blend3Job, BlendTree> {
    Pose pose;
public:
    void onInit(BlendTree* bt) override;
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
        if (_a->sample_buffer.sampleCount() == 0 || _b->sample_buffer.sampleCount() == 0) {
            return;
        }

        for(size_t i = 0; i < pose.sample_buffer.sampleCount(); ++i) {
            pose.sample_buffer[i].t = gfxm::lerp(_a->sample_buffer[i].t, _b->sample_buffer[i].t, lr_weight);
            pose.sample_buffer[i].r = gfxm::slerp(_a->sample_buffer[i].r, _b->sample_buffer[i].r, lr_weight);
            pose.sample_buffer[i].s = gfxm::lerp(_a->sample_buffer[i].s, _b->sample_buffer[i].s, lr_weight);
        }
        pose.speed = gfxm::lerp(_a->speed, _b->speed, lr_weight);

        pose.sample_buffer.getRootMotionDelta().t = gfxm::lerp(_a->sample_buffer.getRootMotionDelta().t, _b->sample_buffer.getRootMotionDelta().t, lr_weight);
    }
};


class SingleAnimJob : public JobNode<SingleAnimJob, BlendTree> {
    std::shared_ptr<Animation> anim;
    std::vector<int32_t> mapping;
    Pose pose;
    bool ready = false;

    void tryInit();

public:
    SingleAnimJob();

    void onInit(BlendTree*) override;
    void onInvoke() override;

    void onGui() override;

    void write(out_stream& out) override {
        DataWriter w(&out);
        if(anim) {
            w.write(anim->Name());
        } else {
            w.write(std::string());
        }
    }
    void read(in_stream& in) override {
        DataReader r(&in);
        std::string anim_name = r.readStr();
        anim = retrieve<Animation>(anim_name);
    }
};


class PoseResultJob : public JobNode<PoseResultJob, BlendTree> {
public:
    void onInit(BlendTree*) override {}
    void onInvoke() override;
};


class FloatNode : public JobNode<FloatNode, BlendTree> {
    float v;
    std::string value_name;
    int value_index = -1;
public:
    void onInit(BlendTree* bt);
    void onInvoke();

    void onGui();
};


class MotionParam : public JobNode<MotionParam, BlendTree> {
    float v = .0f;
    std::string param_name = "";
    int index = -1;
public:
    void onInit(BlendTree* bt);
    void onInvoke();

    void onGui();

    void write(out_stream& out) override;
    void read(in_stream& in) override;
};



#endif
