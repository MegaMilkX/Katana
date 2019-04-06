#ifndef SAMPLE_ATTRIB_HPP
#define SAMPLE_ATTRIB_HPP

#include "attribute.hpp"

class SampleAttrib : public Attribute {
    RTTR_ENABLE(Attribute)
public:
};
STATIC_RUN(SampleAttrib) {
    reg_type<SampleAttrib>("SampleAttrib");
}

#endif
