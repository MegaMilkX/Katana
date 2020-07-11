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

class BlendAddJob : public JobNode<BlendAddJob, BlendTree> {
    Pose pose;

public:
    void onInit(BlendTree* bt) override;
    void onInvoke() override {
        Pose& target = get<Pose>(0);
        Pose& p = get<Pose>(1);
        Pose& ref = get<Pose>(2);

        auto& p_samples = p.sample_buffer.getSamples();
        auto& ref_samples = ref.sample_buffer.getSamples();
        auto& tgt_samples = target.sample_buffer.getSamples();

        for(int i = 0; i < p_samples.size() && i < ref_samples.size(); ++i) {
            auto& q = p_samples[i].r;
            auto& ref_q = ref_samples[i].r;
            auto& tgt_q = tgt_samples[i].r;
            auto& t = p_samples[i].t;
            auto& ref_t = ref_samples[i].t;
            auto& tgt_t = tgt_samples[i].t;

            pose.sample_buffer[i].r = gfxm::inverse(ref_q) * q * tgt_q;
            pose.sample_buffer[i].t = t - ref_t + tgt_t;
        }
    }

};

class Blend2Job : public JobNode<Blend2Job, BlendTree> {
    Pose pose;
    std::vector<std::string> blend_root_bones;
    std::vector<float> filter_weights;

    void reinit();

public:
    void onInit(BlendTree* bt) override;
    void onInvoke() override {
        Pose& a = get<Pose>(0);
        Pose& b = get<Pose>(1);
        float w = get<float>(2);
        if(w < .0f) {
            w = .0f;
        }
        if(w > 1.0f) {
            w = 1.0f;
        }

        if (a.sample_buffer.sampleCount() == 0 || b.sample_buffer.sampleCount() == 0) {
            return;
        }

        for(size_t i = 0; i < pose.sample_buffer.sampleCount(); ++i) {
            float fw = filter_weights[i];
            pose.sample_buffer[i].t = gfxm::lerp(a.sample_buffer[i].t, b.sample_buffer[i].t, w * fw);
            pose.sample_buffer[i].r = gfxm::slerp(a.sample_buffer[i].r, b.sample_buffer[i].r, w * fw);
            pose.sample_buffer[i].s = gfxm::lerp(a.sample_buffer[i].s, b.sample_buffer[i].s, w * fw);
        }
        if(a.speed == .0f) {
            pose.speed = b.speed;
        } else if(b.speed == .0f) {
            pose.speed = a.speed;
        } else {
            pose.speed = gfxm::lerp(a.speed, b.speed, w);
        }

        pose.sample_buffer.getRootMotionDelta().t = gfxm::lerp(a.sample_buffer.getRootMotionDelta().t, b.sample_buffer.getRootMotionDelta().t, w);
    }

    void onGui() override;

    void write(out_stream& out) override {
        DataWriter w(&out);
        w.write<uint32_t>(blend_root_bones.size());
        for(int i = 0; i < blend_root_bones.size(); ++i) {
            w.write(blend_root_bones[i]);
        }
    }
    void read(in_stream& in) override {
        DataReader r(&in);
        uint32_t count = r.read<uint32_t>();
        for(int i = 0; i < count; ++i) {
            blend_root_bones.push_back(r.readStr());
        }
    }
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

class SingleAnimPoseJob : public JobNode<SingleAnimPoseJob, BlendTree> {
    std::shared_ptr<Animation> anim;
    std::vector<int32_t>       mapping;
    Pose                       pose;
    bool                       ready = false;

    void tryInit();

public:
    void onInit(BlendTree* bt) override;
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
    float v = .5f;
    std::string value_name;
    int value_index = -1;
public:
    void onInit(BlendTree* bt);
    void onInvoke();

    void onGui();

    void write(out_stream& out) override {

    }
    void read(in_stream& in) override {
        
    }
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
