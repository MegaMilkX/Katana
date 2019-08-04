#ifndef RENDER_ENVIRONMENT_HPP
#define RENDER_ENVIRONMENT_HPP

#include "component.hpp"
#include "../util/animation/curve.h"
#include "../gfxm.hpp"

class RenderEnvironment : public Attribute {
    RTTR_ENABLE(Attribute)

    curve<gfxm::vec3> gradient;
public:
    void              setSkyGradient(const curve<gfxm::vec3> g) { gradient = g; }
    curve<gfxm::vec3> getSkyGradient() { return gradient; }

    void onGui() override;

    bool serialize(out_stream& out) override;
    bool deserialize(in_stream& in, size_t sz) override;
};
REG_ATTRIB(RenderEnvironment);

#endif
