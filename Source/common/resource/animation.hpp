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

class Animation : public Resource {
    RTTR_ENABLE(Resource)
public:
    struct event {
        float time;
        float threshold;
    };

    float length = 0.0f;
    float fps = 60.0f;
    std::string root_motion_node_name;
    bool root_motion_enabled = false;

    void                 clearNodes();
    size_t               nodeCount();
    AnimNode&            getNode(const std::string& name);
    int32_t              getNodeIndex(const std::string& name);
    AnimNode&            getRootMotionNode();

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

    virtual void         serialize(out_stream& out);
    virtual bool         deserialize(in_stream& in, size_t sz);

    virtual const char*  getWriteExtension() const { return "anm"; }
private:
    AnimNode root_motion_node;
    std::vector<AnimNode> nodes;
    std::map<std::string, size_t> node_names;
    std::map<std::string, curve<float>> extra_curves;
    std::map<std::string, event> events;

    std::map<Skeleton*, std::vector<int32_t>> mappings;
};

class Skeleton;
void buildAnimSkeletonMapping(Animation* anim, Skeleton* skel, std::vector<int32_t>& bone_mapping);

#endif
