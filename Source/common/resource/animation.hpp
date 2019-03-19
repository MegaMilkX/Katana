#ifndef ANIMATION_HPP
#define ANIMATION_HPP

#include "resource.h"

#include "../util/animation/curve.h"
#include "../util/zip_reader.hpp"
#include "../util/zip_writer.hpp"

#include "skeleton.hpp"

struct AnimSample {
    gfxm::vec3 t;
    gfxm::quat r;
    gfxm::vec3 s;
};

struct AnimNode {
    curve<gfxm::vec3> t;
    curve<gfxm::quat> r;
    curve<gfxm::vec3> s;
};

class Animation : public Resource {
public:
    float length = 0.0f;
    float fps = 60.0f;
    std::string root_motion_node_name;
    bool root_motion_enabled = false;

    size_t              nodeCount();
    AnimNode&           getNode(const std::string& name);
    int32_t             getNodeIndex(const std::string& name);
    AnimNode&           getRootMotionNode();

    bool                curveExists(const std::string& name);
    curve<float>&       getCurve(const std::string& name);
    size_t              curveCount() const;
    curve<float>&       getCurve(size_t i);
    const std::string&  getCurveName(size_t i);
    void                removeCurve(const std::string& name);

    void                sample_remapped(
        std::vector<AnimSample>& out,
        float cursor,
        const std::map<size_t, size_t>& remap
    );
    void                sample_remapped(
        std::vector<AnimSample>& samples,
        float cursor,
        const std::vector<size_t>& remap
    );

    void                blend_remapped(
        std::vector<AnimSample>& in_out,
        float cursor, float weight,
        const std::map<size_t, size_t>& remap
    );
    void                blend_remapped(
        std::vector<AnimSample>& samples,
        float cursor, float weight,
        const std::vector<size_t>& remap
    );

    void                additive_blend_remapped(
        std::vector<AnimSample>& in_out,
        float cursor, float weight,
        const std::map<size_t, size_t>& remap
    );
    void                additive_blend_remapped(
        std::vector<AnimSample>& samples,
        float cursor, float weight,
        const std::vector<size_t>& remap
    );

    virtual void        serialize(out_stream& out);
    virtual bool        deserialize(std::istream& in_, size_t sz);
private:
    AnimNode root_motion_node;
    std::vector<AnimNode> nodes;
    std::map<std::string, size_t> node_names;
    std::map<std::string, curve<float>> extra_curves;
    std::vector<std::pair<float, std::string>> events;
};

#endif
