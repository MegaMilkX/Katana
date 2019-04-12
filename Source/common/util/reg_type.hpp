#ifndef REGISTER_TYPE_HPP
#define REGISTER_TYPE_HPP

#include <rttr/type>
#include <rttr/registration>
#include "static_run.h"

#define REG_TYPE(TYPE) \
STATIC_RUN(TYPE) { \
    rttr::registration::class_<TYPE>(#TYPE) \
        .constructor<>()( \
            rttr::policy::ctor::as_raw_ptr \
        ); \
}

#endif
