#ifndef KT_ACTOR_HPP
#define KT_ACTOR_HPP

#include "../attribute.hpp"

class ktWorld;
class ktActor : public Attribute {
    RTTR_ENABLE(Attribute)
public:
    virtual void onStart(ktWorld*) {}
    virtual void onUpdate() {}
    virtual void onCleanup() {}
};

#endif
