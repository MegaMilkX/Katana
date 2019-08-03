#ifndef KT_ACTOR_HPP
#define KT_ACTOR_HPP

#include "../component.hpp"
//#include "../../world.hpp"

class ktWorld;
class ktActor : public Attribute {
    RTTR_ENABLE(Attribute)
public:
    virtual void onStart(ktWorld*) {}
    virtual void onUpdate() {}
    virtual void onCleanup() {}
};

#endif
