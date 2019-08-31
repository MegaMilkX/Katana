#ifndef ARG_INFO_HPP
#define ARG_INFO_HPP

#include <rttr/type>

enum ARG_TYPE {
    ARG_NONE,
    ARG_IN,
    ARG_OUT
};
struct ArgInfo {
    rttr::type type;
    size_t sz;
    ARG_TYPE arg_type;
    size_t in_out_index;
};

#endif
