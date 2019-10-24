#ifndef ECS_ARCHETYPE_HPP
#define ECS_ARCHETYPE_HPP

#include <tuple>

#include "entity.hpp"

class ecsArchetypeBase {
public:
    virtual ~ecsArchetypeBase() {}
    virtual uint64_t get_signature() const = 0;
};
template<typename Arg>
class ecsArchetypePart {
    Arg* ptr;
public:
    virtual ~ecsArchetypePart() {}
};
template<typename... Args>
class ecsArchetype : public ecsArchetypeBase {
    std::tuple<Args*...> attribs;
public:
    ecsArchetype() {}
    ecsArchetype(ecsEntity* ent) {
        attribs = std::tuple<Args*...>(ent->getAttrib<Args>()...);
    }

    template<typename T>
    T* get() {
        return std::get<T*>(attribs);
    }

    static uint64_t get_signature_static() {
        static uint64_t x[] = { Args::get_id_static()... };
        static uint64_t sig = 0;
        for(size_t i = 0; i < sizeof...(Args); ++i) {
            sig |= (1 << x[i]);
        }
        return sig;
    }
    uint64_t get_signature() const override {
        return get_signature_static();
    }

    template<typename T>
    T makeStruct(ecsEntity* ent) {
        return T { ent->getAttrib<Args>()... };
    }

};

#endif
