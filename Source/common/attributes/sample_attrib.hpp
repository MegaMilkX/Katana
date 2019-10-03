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
    void write(SceneWriteCtx& out) override {

    }
    void read(SceneReadCtx& in) override {
        
    }
};

#endif
