#ifndef SAMPLE_ATTRIB_HPP
#define SAMPLE_ATTRIB_HPP

#include "attribute.hpp"

class SampleAttrib : public Attribute {
    RTTR_ENABLE(Attribute)
public:
    virtual void copy(const SampleAttrib& other) {

    }
    virtual void onGui() {
        ImGui::Text("Sample attribute");
    }
    virtual bool serialize(out_stream& out) {
        return true;
    }
    virtual bool deserialize(in_stream& in, size_t sz) {
        return true;
    }
};
REG_ATTRIB(SampleAttrib, SampleAttrib, Samples);

#endif
