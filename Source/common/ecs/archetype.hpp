#ifndef ECS_ARCHETYPE_HPP
#define ECS_ARCHETYPE_HPP

#include <tuple>

#include "entity.hpp"

class ecsArchetypeBase {
public:
    virtual ~ecsArchetypeBase() {}
    virtual uint64_t get_signature() const = 0;
    virtual uint64_t get_exclusion_signature() const = 0;

    virtual void signalAttribUpdate(uint64_t attrib_sig) = 0;
};
template<typename Arg>
class ecsArchetypePart {
protected:
    Arg* ptr;
public:
    ecsArchetypePart(ecsEntity* ent) {
        ptr = ent->findAttrib<Arg>();
    }
    virtual ~ecsArchetypePart() {}

    static uint64_t get_inclusion_sig() {
        return 1 << Arg::get_id_static();
    }
    static uint64_t get_exclusion_sig() {
        return 0;
    }

    int _signalAttribUpdate(uint64_t attrib_sig) {
        if((1 << Arg::get_id_static()) == attrib_sig) {
            onAttribUpdate(ptr);
        }
        return 0;
    }

    virtual void onAttribUpdate(Arg* arg) {}
};
template<typename Arg>
class ecsArchetypePart<ecsExclude<Arg>> {
public:
    ecsArchetypePart(ecsEntity* ent) {}
    virtual ~ecsArchetypePart() {}

    static uint64_t get_inclusion_sig() {
        return 0;
    }
    static uint64_t get_exclusion_sig() {
        return 1 << Arg::get_id_static();
    }

    int _signalAttribUpdate(uint64_t attrib_sig) {
        return 0;
    }
};
template<typename... Args>
class ecsArchetype : public ecsArchetypeBase, public ecsArchetypePart<Args>... {
public:
    ecsArchetype() {}
    ecsArchetype(ecsEntity* ent)
    : ecsArchetypePart<Args>(ent)... {
        //(ecsArchetypePart<Args>::ptr... = ent->getAttrib<Args>()...);
        //int x[] = { foo<Args>(ent)... };
        //attribs = std::tuple<Args*...>(ent->getAttrib<Args>()...);
    }

    template<typename T>
    T* get() {
        return ecsArchetypePart<T>::ptr;
        //return std::get<T*>(attribs);
    }

    void signalAttribUpdate(uint64_t attrib_sig) {
        int x[] = { ecsArchetypePart<Args>::_signalAttribUpdate(attrib_sig)... };
    }

    static uint64_t get_signature_static() {
        static uint64_t x[] = { ecsArchetypePart<Args>::get_inclusion_sig()... };
        static uint64_t sig = 0;
        for(size_t i = 0; i < sizeof...(Args); ++i) {
            sig |= x[i];
        }
        return sig;
    }
    uint64_t get_signature() const override {
        return get_signature_static();
    }

    static uint64_t get_exclusion_signature_static() {
        static uint64_t x[] = { ecsArchetypePart<Args>::get_exclusion_sig()... };
        static uint64_t sig = 0;
        for(size_t i = 0; i < sizeof...(Args); ++i) {
            sig |= x[i];
        }
        return sig;
    }
    uint64_t get_exclusion_signature() const override {
        return get_exclusion_signature_static();
    }
};

#endif
