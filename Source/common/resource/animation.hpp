#ifndef ANIMATION_HPP
#define ANIMATION_HPP

#include "resource.h"

#include "../util/animation/curve.h"
#include "../util/zip_reader.hpp"
#include "../util/zip_writer.hpp"

#include "skeleton.hpp"

struct AnimNode {
    curve<gfxm::vec3> t;
    curve<gfxm::quat> r;
    curve<gfxm::vec3> s;
};

class AnimSampleBuffer {
    Skeleton* skeleton = 0;
    std::vector<AnimSample> samples;
    AnimSample root_motion_delta_sample;

public:
    AnimSampleBuffer() {
        root_motion_delta_sample.s = gfxm::vec3(0,0,0);
    }
    AnimSampleBuffer(Skeleton* skeleton) {
        this->skeleton = skeleton;
        samples = skeleton->makePoseArray();
        root_motion_delta_sample.s = gfxm::vec3(0,0,0);
    }
    int sampleCount() const {
        return samples.size();
    }
    std::vector<AnimSample>& getSamples() {
        return samples;
    }
    AnimSample& operator[](int i) {
        return samples[i];
    }
    const AnimSample& operator[](int i) const {
        return samples[i];
    }
    AnimSample& getRootMotionDelta() {
        return root_motion_delta_sample;
    }
    const AnimSample& getRootMotionDelta() const {
        return root_motion_delta_sample;
    }
};

class Animation : public Resource {
    RTTR_ENABLE(Resource)

public:
    struct event {
        float time;
        float threshold;
    };

private:
    std::vector<AnimNode>                       nodes;
    AnimNode                                    root_motion_node;
    std::map<std::string, size_t>               node_indices;
    std::vector<std::string>                    node_names;
    std::map<std::string, curve<float>>         extra_curves;
    std::map<std::string, event>                events;

    std::map<Skeleton*, std::vector<int32_t>>   mappings;

public:
    float           length = 0.0f;
    float           fps = 60.0f;
    std::string     root_motion_node_name;
    int             root_motion_node_index = -1;
    bool            root_motion_enabled = false;
    bool            enable_root_motion_t_xz = false;
    bool            enable_root_motion_t_y = false;
    bool            enable_root_motion_r_y = false;

    void                 clearNodes();
    size_t               nodeCount();
    AnimNode&            getNode(const std::string& name);
    int32_t              getNodeIndex(const std::string& name);
    const std::string&   getNodeName(int i);
    AnimNode&            getRootMotionNode();
    void                 setRootMotionSourceNode(const std::string& name);

    bool                 curveExists(const std::string& name);
    curve<float>&        getCurve(const std::string& name);
    size_t               curveCount() const;
    curve<float>&        getCurve(size_t i);
    const std::string&   getCurveName(size_t i);
    void                 removeCurve(const std::string& name);

    void                 setEvent(const std::string& name, float time, float threshold = 0.0f);
    size_t               eventCount();
    std::string          getEventName(size_t i);
    event&               getEvent(size_t i);
    void                 removeEvent(const std::string& e);
    void                 fireEvents(float from, float to, std::function<void(const std::string& name)> cb, float threshold = 1.0f);

    std::vector<int32_t>& getMapping(Skeleton* skel);
    std::vector<int32_t>  makeMapping(Skeleton* skel, int root_bone_id);
    std::vector<int32_t>  makeMapping(Skeleton* skel, const char* root_bone_name);

    void                 sample_one(int node_id, float cursor, AnimSample& sample);

    void                 sample_remapped(
        AnimSampleBuffer& sample_buffer,
        float prev_cursor,
        float cursor,
        Skeleton* skeleton,
        const std::vector<int32_t>& remap
    );

    void                 sample_remapped(
        std::vector<AnimSample>& out,
        float cursor,
        const std::map<size_t, size_t>& remap
    );
    void                 sample_remapped(
        std::vector<AnimSample>& samples,
        float cursor,
        const std::vector<int32_t>& remap
    );

    void                 blend_remapped(
        std::vector<AnimSample>& in_out,
        float cursor, float weight,
        const std::map<size_t, size_t>& remap
    );
    void                 blend_remapped(
        std::vector<AnimSample>& samples,
        float cursor, float weight,
        const std::vector<int32_t>& remap
    );

    void                 additive_blend_remapped(
        std::vector<AnimSample>& in_out,
        float cursor, float weight,
        const std::map<size_t, size_t>& remap
    );
    void                 additive_blend_remapped(
        std::vector<AnimSample>& samples,
        float cursor, float weight,
        const std::vector<int32_t>& remap
    );

    void                 calcRootMotion(float from, float to, AnimSample& delta);

    virtual void         serialize(out_stream& out);
    virtual bool         deserialize(in_stream& in, size_t sz);

    virtual const char*  getWriteExtension() const { return "anm"; }

};

class Skeleton;
void buildAnimSkeletonMapping(Animation* anim, Skeleton* skel, std::vector<int32_t>& bone_mapping);

#endif
