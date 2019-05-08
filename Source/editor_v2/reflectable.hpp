#ifndef REFLECTABLE_HPP
#define REFLECTABLE_HPP

#include "../common/util/static_run.h"

class Reflectable {
public:
    virtual ~Reflectable() {}
    virtual void onRegister() = 0;
};

#define REG_REFLECTABLE(TYPE) \
STATIC_RUN(TYPE) { \
    TYPE().onRegister(); \
}

#endif
