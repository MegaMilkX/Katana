#ifndef MODEL_HPP
#define MODEL_HPP

#include "component.hpp"

#include "../../common/resource/mesh.hpp"
#include "../../common/resource/material.hpp"

class CmModel : public ObjectComponent {
    RTTR_ENABLE(ObjectComponent)
public:
    struct SkinData {
        std::vector<TransformNode*> bone_nodes;
        std::vector<gfxm::mat4> bind_transforms;
    };
    struct Segment {
        std::shared_ptr<Mesh> mesh;
        std::shared_ptr<Material> material;
        std::shared_ptr<SkinData> skin_data;
    };

    virtual void copy(ObjectComponent* other) {
        if(other->get_type() != get_type()) {
            LOG("Can't copy from " << other->get_type().get_name().to_string() << " to " <<
                get_type().get_name().to_string());
            return;
        }
        CmModel* o = (CmModel*)other;
        for(size_t i = 0; i < o->segmentCount(); ++i) {
            getSegment(i).mesh = o->getSegment(i).mesh;
            getSegment(i).material = o->getSegment(i).material;
            // TODO: Skin data!
        }
    }

    Segment& getSegment(size_t i) {
        if(i >= segments.size()) {
            segments.resize(i + 1);
        }
        return segments[i];
    }
    size_t segmentCount() const {
        return segments.size();
    }
private:
    std::vector<Segment> segments;
};
STATIC_RUN(CmModel) {
    rttr::registration::class_<CmModel>("CmModel")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
