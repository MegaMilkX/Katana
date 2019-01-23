#ifndef LIGHT_SOURCE_HPP
#define LIGHT_SOURCE_HPP

#include "component.hpp"

class LightOmni : public Component {
    CLONEABLE
public:
    
};
STATIC_RUN(LightOmni)
{
    rttr::registration::class_<LightOmni>("LightOmni")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
