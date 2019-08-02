#ifndef ATTRIB_INSTANCEABLE_HPP
#define ATTRIB_INSTANCEABLE_HPP

#include "component.hpp"
#include "../property.hpp"

template<typename T>
class AttribInstantiable : public Attribute {
    RTTR_ENABLE(Attribute)
public:
    virtual void instantiate(T& other) {
        auto owner_cache = owner;
        *(T*)this = other;
        owner = owner_cache; // Keep owner pointer from changing even if copying everything else
    }
private:
    virtual void instantiate(Attribute& other) override {
        instantiate(dynamic_cast<T&>(other));
    }
};

#endif
